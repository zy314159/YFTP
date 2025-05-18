#pragma once
#include <string>
#include <vector>
#include <zlib.h>

class CompressUtil {
public:
    static bool compress(const std::string& input, std::string& output) {
        uLongf destLen = compressBound(input.size());
        std::vector<char> buffer(destLen);
        int res = ::compress(reinterpret_cast<Bytef*>(buffer.data()), &destLen,
                             reinterpret_cast<const Bytef*>(input.data()), input.size());
        if (res != Z_OK) return false;
        output.assign(buffer.data(), destLen);
        return true;
    }

    static bool decompress(const std::string& input, std::string& output, uLongf original_size) {
        std::vector<char> buffer(original_size);
        int res = ::uncompress(reinterpret_cast<Bytef*>(buffer.data()), &original_size,
                               reinterpret_cast<const Bytef*>(input.data()), input.size());
        if (res != Z_OK) return false;
        output.assign(buffer.data(), original_size);
        return true;
    }

    static std::string base64_encode(const std::string& in) {
        static const char table[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
        std::string out;
        int val=0, valb=-6;
        for (uint8_t c : in) {
            val = (val<<8) + c;
            valb += 8;
            while (valb>=0) {
                out.push_back(table[(val>>valb)&0x3F]);
                valb-=6;
            }
        }
        if (valb>-6) out.push_back(table[((val<<8)>>(valb+8))&0x3F]);
        while (out.size()%4) out.push_back('=');
        return out;
    }
    // base64解码
    static std::string base64_decode(const std::string& in) {
        static const char table[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
        static std::vector<int> T(256,-1);
        for (int i=0; i<64; i++) T[table[i]] = i;
        std::string out;
        int val=0, valb=-8;
        for (uint8_t c : in) {
            if (T[c] == -1) break;
            val = (val<<6) + T[c];
            valb += 6;
            if (valb>=0) {
                out.push_back(char((val>>valb)&0xFF));
                valb-=8;
            }
        }
        return out;
    }
};