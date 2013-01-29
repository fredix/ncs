#include "util.h"



long strtolong(const std::string& str) {
    std::istringstream stream (str);
    long i = 0;
    stream >> i;
    return i;
}

long long strtolonglong(const std::string& str) {
    std::istringstream stream (str);
    long long i = 0;
    stream >> i;
    return i;
}


std::string inttostr(const int i) {
    std::string str;
    std::stringstream out;
    out << i;
    str = out.str();
    return str;
}

std::string hex_decode(const std::string &in) {
    std::string out;
    out.reserve(20);
    unsigned int in_length = in.length();
    for(unsigned int i = 0; i < in_length; i++) {
        unsigned char x = '0';
        if(in[i] == '%' && (i + 2) < in_length) {
            i++;
            if(in[i] >= 'a' && in[i] <= 'f') {
                x = static_cast<unsigned char>((in[i]-87) << 4);
            } else if(in[i] >= 'A' && in[i] <= 'F') {
                x = static_cast<unsigned char>((in[i]-55) << 4);
            } else if(in[i] >= '0' && in[i] <= '9') {
                x = static_cast<unsigned char>((in[i]-48) << 4);
            }

            i++;
            if(in[i] >= 'a' && in[i] <= 'f') {
                x += static_cast<unsigned char>(in[i]-87);
            } else if(in[i] >= 'A' && in[i] <= 'F') {
                x += static_cast<unsigned char>(in[i]-55);
            } else if(in[i] >= '0' && in[i] <= '9') {
                x += static_cast<unsigned char>(in[i]-48);
            }
        } else {
            x = in[i];
        }
        out.push_back(x);
    }
    return out;
}

std::string hextostr(const std::string &in)
{
    std::string out;
        const char *hex = in.c_str();
        unsigned int ch;
        for(;sscanf(hex, "%2x", &ch) == 1; hex+=2)
           out += ch;
        return out;
}



//extract key on get URL
QString getkey(QUrl url, QString key, bool &error, bool fixed_size) {
    QString val = url.queryItemValue(key);
    QString msg;

    if (!error && val.isEmpty ())
    {
        error = true;
        msg = "Invalid request, missing data";
        msg.prepend("d14:failure reason msg.size():");
        msg.append("e");
        return msg;

    }
    else if (!error && fixed_size && val.size () != 20)
    {
        error = true;
        msg = "Invalid request, length on fixed argument not correct";
        msg.prepend("d14:failure reason msg.size():");
        msg.append("e");
        return msg;
    }
    else if (!error && val.size () > 80)
    { //128 chars should really be enough
        error = true;
        msg = "Request too long";
        msg.prepend("d14:failure reason msg.size():");
        msg.append("e");
        return msg;
    }

    if (val.isEmpty() && (key == "downloaded" || key == "uploaded" || key == "left"))
            val="0";

    return val;
}
