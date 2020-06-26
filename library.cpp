#include "library.h"

#define  L_FRAME            80
#define  L_FRAME_COMPRESSED 10
typedef int16_t g729_block[L_FRAME];
typedef uint8_t g729_frame[L_FRAME_COMPRESSED];

TranslatorCaps G729Plugin::caps[] = {
        { nullptr, nullptr },
        { nullptr, nullptr },
        { nullptr, nullptr },
};

volatile int G729Codec::count = 0;

G729Plugin::G729Plugin()
: Plugin("g729codec") {
    Output("Loaded module G729 - based on Belledonne Communications");
    const FormatInfo* f = FormatRepository::addFormat("g729", L_FRAME_COMPRESSED, L_FRAME * 1000 / 8);
    caps[0].src = caps[1].dest = f;
    caps[0].dest = caps[1].src = FormatRepository::getFormat("slin");
    caps[0].cost = caps[1].cost = 5;
}

G729Plugin::~G729Plugin() {
    Output("Unloading module G729");
}

bool G729Plugin::isBusy() const {
    return (G729Codec::count != 0);
}

DataTranslator *G729Plugin::create(const DataFormat& sFormat, const DataFormat& dFormat) {
    if (sFormat == "slin" && dFormat == "g729")
        return new G729Codec(sFormat,dFormat,true);
    else if (sFormat == "g729" && dFormat == "slin")
        return new G729Codec(sFormat,dFormat,false);
    else return nullptr;
}

const TranslatorCaps *G729Plugin::getCapabilities() const {
    return caps;
}

G729Codec::G729Codec(const char *sFormat, const char *dFormat, bool encoding)
: DataTranslator(sFormat, dFormat)
, encoder(nullptr)
, decoder(nullptr) {
    Debug(DebugAll,R"(G729Codec::G729Codec("%s","%s",%scoding) [%p])",
          sFormat,dFormat, encoding ? "en" : "de", this);

    if (encoding) {
        encoder = initBcg729EncoderChannel(0);
    }
    else {
        decoder = initBcg729DecoderChannel();
    }
    // gcc way to say ++count atomically
    __sync_add_and_fetch(&count, 1);
}

G729Codec::~G729Codec() {
    if (encoder) {
        auto tmp = encoder;
        encoder = nullptr;
        closeBcg729EncoderChannel(tmp);
    }
    if (decoder) {
        auto tmp = decoder;
        decoder = nullptr;
        closeBcg729DecoderChannel(tmp);
    }
    __sync_add_and_fetch(&count, -1);
}

unsigned long G729Codec::Consume(const DataBlock& data, unsigned long tStamp, unsigned long flags) {
    if (!getTransSource())
        return 0;
    if (data.null() && (flags & DataSilent))
        return getTransSource()->Forward(data, tStamp, flags);
    ref();
    m_data += data;
    DataBlock outdata;
    size_t frames, consumed;
    if (encoder) {
        frames = m_data.length() / sizeof(g729_block);
        consumed = frames * sizeof(g729_block);
        if (frames) {
            outdata.assign(nullptr, frames * sizeof(g729_frame));
            for (size_t i = 0; i < frames; ++i) {
                auto decompressed_buffer = (int16_t *)(((g729_block *)m_data.data()) + i);
                auto compressed_buffer = (uint8_t *)(((g729_frame *)outdata.data()) + i);
                u_int8_t bitStreamLength;
                bcg729Encoder(encoder, decompressed_buffer, compressed_buffer, &bitStreamLength);
            }
        }
        if (!tStamp)
            tStamp = timeStamp() + (consumed / 2);
    }
    else {
        frames = m_data.length() / sizeof(g729_frame);
        consumed = frames * sizeof(g729_frame);
        if (frames) {
            outdata.assign(nullptr, frames*sizeof(g729_block));
            for (size_t i = 0; i < frames; ++i) {
                auto compressed_buffer = (uint8_t *)(((g729_frame *)m_data.data()) + i);
                auto decompressed_buffer = (int16_t *)(((g729_block *)outdata.data()) + i);
                bcg729Decoder(decoder, compressed_buffer, L_FRAME_COMPRESSED, 0, 0, 0, decompressed_buffer);
            }
        }
        if (!tStamp)
            tStamp = timeStamp() + (frames*sizeof(g729_block) / 2);
    }
    XDebug("G729Codec",DebugAll,"%scoding %d frames of %d input bytes (consumed %d) in %d output bytes",
           encoder ? "en" : "de",frames,m_data.length(),consumed,outdata.length());
    unsigned long len = 0;
    if (frames) {
        m_data.cut(-consumed);
        len = getTransSource()->Forward(outdata, tStamp, flags);
    }
    deref();
    return len;
}

INIT_PLUGIN(G729Plugin);
