#ifndef G729_YATE_LIBRARY_H
#define G729_YATE_LIBRARY_H

#include <yatephone.h>

extern "C" {
#include <bcg729/encoder.h>
#include <bcg729/decoder.h>
}

using namespace TelEngine;

class G729Plugin : public Plugin, public TranslatorFactory {
public:
    G729Plugin();
    ~G729Plugin() override;
    void initialize() override { }
    bool isBusy() const override;
    DataTranslator* create(const DataFormat& sFormat, const DataFormat& dFormat) override;
    const TranslatorCaps* getCapabilities() const override;

private:
    static TranslatorCaps caps[];
};

class G729Codec : public DataTranslator {
public:
    G729Codec(const char* sFormat, const char* dFormat, bool encoding);
    ~G729Codec() override;
    unsigned long Consume(const DataBlock& data, unsigned long tStamp, unsigned long flags) override;
    volatile static int count;

private:
    DataBlock m_data;
    bcg729EncoderChannelContextStruct* encoder;
    bcg729DecoderChannelContextStruct* decoder;
};

#endif //G729_YATE_LIBRARY_H
