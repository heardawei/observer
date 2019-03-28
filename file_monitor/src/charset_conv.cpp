/*
 * =====================================================================================
 *
 *       Filename:  charset_conv.cpp
 *
 *    Description:  j
 *
 *        Version:  1.0
 *        Created:  05/03/2019 10:51:47 AM
 *       Revision:  none
 *       Compiler:  g++
 *
 *         Author:  Lidawei
 *   Organization:
 *
 * =====================================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <inttypes.h>
#include <iconv.h>
#include <string>
#include <sstream>

#define MAX_UTF8_UNIT_BYTES 6

static inline uint8_t get_uint8(uint8_t high, uint8_t low)
{
    uint8_t ret;

    if (high - '0' < 10)
        ret = high - '0';
    else if (high - 'A' < 6)
        ret = high - 'A' + 0x0A;
    else if (high - 'a' < 6)
        ret = high - 'a' + 0x0A;

    ret = ret << 4;

    if (low - '0' < 10)
        ret |= low - '0';
    else if (low - 'A' < 6)
        ret |= low - 'A' + 0x0A;
    else if (low - 'a' < 6)
        ret |= low - 'a' + 0x0A;
    return  ret;
}

static std::string wstring_to_utf8(const std::wstring& str)
{
    iconv_t cd = iconv_open("UTF-8", "WCHAR_T");
    if(cd == (iconv_t)-1)
    {
        fprintf(stderr, "%s conv_open(%s, %s) failed, errno:%d\n", __func__, "UTF-8", "UTF-16LE", errno);
        return "";
    }
    char *src = (char *)str.data();
    size_t srcsz = str.size() * sizeof(str[0]);

    std::string _dst;
    _dst.resize(srcsz * MAX_UTF8_UNIT_BYTES, 0x00);
    char *dst = (char *)_dst.data();
    size_t dstsz = _dst.size();

    // printf("srcsz:%lu, dstsz:%lu\n", srcsz, dstsz);
    size_t res = iconv(cd, (char **)&src, &srcsz, &dst, &dstsz);
    // printf("srcsz:%lu, dstsz:%lu, res:%d, errno:%d\n", srcsz, dstsz, (int)res, errno);
    iconv_close(cd);
    if(res == (size_t)-1)
    {
        return "";
    }
    return _dst.substr(0, _dst.size() - dstsz);
}

static std::string u32string_to_utf8(const std::u32string& str)
{
    iconv_t cd = iconv_open("UTF-8", "UTF-32LE");
    if(cd == (iconv_t)-1)
    {
        fprintf(stderr, "%s conv_open(%s, %s) failed, errno:%d\n", __func__, "UTF-8", "UTF-32LE", errno);
        return "";
    }
    char *src = (char *)str.data();
    size_t srcsz = str.size() * sizeof(str[0]);

    std::string _dst;
    _dst.resize(srcsz * MAX_UTF8_UNIT_BYTES, 0x00);
    char *dst = (char *)_dst.data();
    size_t dstsz = _dst.size();

    // printf("srcsz:%lu, dstsz:%lu\n", srcsz, dstsz);
    size_t res = iconv(cd, (char **)&src, &srcsz, &dst, &dstsz);
    // printf("srcsz:%lu, dstsz:%lu, res:%d, errno:%d\n", srcsz, dstsz, (int)res, errno);
    iconv_close(cd);
    if(res == (size_t)-1)
    {
        return "";
    }
    return _dst.substr(0, _dst.size() - dstsz);
}

static std::string u16string_to_utf8(const std::u16string& str)
{
    iconv_t cd = iconv_open("UTF-8", "UTF-16LE");
    if(cd == (iconv_t)-1)
    {
        fprintf(stderr, "%s conv_open(%s, %s) failed, errno:%d\n", __func__, "UTF-8", "UTF-16LE", errno);
        return "";
    }
    char *src = (char *)str.data();
    size_t srcsz = str.size() * sizeof(str[0]);

    std::string _dst;
    _dst.resize(srcsz * MAX_UTF8_UNIT_BYTES, 0x00);
    char *dst = (char *)_dst.data();
    size_t dstsz = _dst.size();

    // printf("srcsz:%lu, dstsz:%lu\n", srcsz, dstsz);
    size_t res = iconv(cd, (char **)&src, &srcsz, &dst, &dstsz);
    // printf("srcsz:%lu, dstsz:%lu, res:%d, errno:%d\n", srcsz, dstsz, (int)res, errno);
    iconv_close(cd);
    if(res == (size_t)-1)
    {
        return "";
    }
    return _dst.substr(0, _dst.size() - dstsz);
}

std::string unicode_escape_string_to_utf8_via_wchar(const std::string source)
{
    std::wstring ws; ws.reserve(source.size());
    std::wstringstream wis(ws);

    auto s = source.begin();
    while (s != source.end())
    {
        if (*s == '\\')
        {
            if (std::distance(s, source.end()) > 5)
            {
                if (*(s + 1) == 'u')
                {
                    wchar_t v = get_uint8(*(s + 2), *(s + 3)) << 8;
                    v |= get_uint8(*(s + 4), *(s + 5));

                    s += 6;
                    wis << v;
                    continue;
                }
            }
        }
        wis << wchar_t(*s);
        s++;
    }
    // for (auto & n : wis.str())
    // {
        // printf("0X%08X ", n);
    // }
    return wstring_to_utf8(wis.str());
}

std::string unicode_escape_string_to_utf8_via_u32(const std::string source)
{
    std::u32string ws; ws.reserve(source.size());
    std::basic_stringstream<char32_t> wis(ws);

    auto s = source.begin();
    while (s != source.end())
    {
        if (*s == '\\')
        {
            if (std::distance(s, source.end()) > 5)
            {
                if (*(s + 1) == 'u')
                {
                    char32_t v = get_uint8(*(s + 2), *(s + 3)) << 8;
                    v |= get_uint8(*(s + 4), *(s + 5));

                    s += 6;
                    wis << v;
                    continue;
                }
            }
        }
        wis << char32_t(*s);
        s++;
    }
    // for (auto & n : wis.str())
    // {
        // printf("0X%08X ", n);
    // }
    return u32string_to_utf8(wis.str());
}

std::string unicode_escape_string_to_utf8_via_u16(const std::string source)
{
    std::u16string ws; ws.reserve(source.size());
    std::basic_stringstream<char16_t> wis(ws);

    auto s = source.begin();
    while (s != source.end())
    {
        if (*s == '\\')
        {
            if (std::distance(s, source.end()) > 5)
            {
                if (*(s + 1) == 'u')
                {
                    char16_t v = get_uint8(*(s + 2), *(s + 3)) << 8;
                    v |= get_uint8(*(s + 4), *(s + 5));

                    s += 6;
                    wis << v;
                    continue;
                }
            }
        }
        wis << char16_t(*s);
        s++;
    }
    // for (auto & n : wis.str())
    // {
        // printf("0X%04X ", n);
    // }
    return u16string_to_utf8(wis.str());
}

bool unicode_to_utf8 (char *inbuf, size_t *inlen, char *outbuf, size_t *outlen)
{
    /* 目的编码, TRANSLIT：遇到无法转换的字符就找相近字符替换
     *           IGNORE ：遇到无法转换字符跳过*/
    const char *encTo = "UTF-8//IGNORE";
    /* 源编码 */
    const char *encFrom = "UCS-2LE";

    /* 获得转换句柄
     *@param encTo 目标编码方式
     *@param encFrom 源编码方式
     *
     * */
    iconv_t cd = iconv_open (encTo, encFrom);
    if (cd == (iconv_t)-1)
    {
        perror ("iconv_open");
    }

    /* 需要转换的字符串 */
    // printf("inbuf=%s\n", inbuf);

    /* 打印需要转换的字符串的长度 */
    // printf("inlen=%d\n", (int)*inlen);


    /* 由于iconv()函数会修改指针，所以要保存源指针 */
    char *tmpin = inbuf;
    char *tmpout = outbuf;
    // size_t insize = *inlen;
    size_t outsize = *outlen;

    /* 进行转换
     *@param cd iconv_open()产生的句柄
     *@param srcstart 需要转换的字符串
     *@param inlen 存放还有多少字符没有转换
     *@param tempoutbuf 存放转换后的字符串
     *@param outlen 存放转换后,tempoutbuf剩余的空间
     *
     * */
    size_t ret = iconv (cd, &tmpin, inlen, &tmpout, outlen);
    if (ret == (size_t)-1)
    {
        perror ("iconv");
    }

    /* 存放转换后的字符串 */
    // printf("outbuf=%s\n", outbuf);

    //存放转换后outbuf剩余的空间
    // printf("outlen=%d\n", (int)*outlen);

    size_t i = 0;

    for (i=0; i<(outsize- (*outlen)); i++)
    {
        // printf("%2c", outbuf[i]);
        // printf("%x\n", outbuf[i]);
    }

    /* 关闭句柄 */
    iconv_close (cd);

    return 0;
}

bool utf8_to_unicode (char *inbuf, size_t *inlen, char *outbuf, size_t *outlen)
{
    /* 目的编码, TRANSLIT：遇到无法转换的字符就找相近字符替换
     *           IGNORE ：遇到无法转换字符跳过*/
    const char *encTo = "UCS-2LE//IGNORE";
    /* 源编码 */
    const char *encFrom = "UTF-8";

    /* 获得转换句柄
     *@param encTo 目标编码方式
     *@param encFrom 源编码方式
     *
     * */
    iconv_t cd = iconv_open (encTo, encFrom);
    if (cd == (iconv_t)-1)
    {
        perror ("iconv_open");
    }

    /* 需要转换的字符串 */
    // printf("inbuf=%s\n", inbuf);

    /* 打印需要转换的字符串的长度 */
    // printf("inlen=%d\n", (int)*inlen);

    /* 由于iconv()函数会修改指针，所以要保存源指针 */
    char *tmpin = inbuf;
    char *tmpout = outbuf;
    // size_t insize = *inlen;
    // size_t outsize = *outlen;

    /* 进行转换
     *@param cd iconv_open()产生的句柄
     *@param srcstart 需要转换的字符串
     *@param inlen 存放还有多少字符没有转换
     *@param tempoutbuf 存放转换后的字符串
     *@param outlen 存放转换后,tempoutbuf剩余的空间
     *
     * */
    size_t ret = iconv (cd, &tmpin, inlen, &tmpout, outlen);
    if (ret == (size_t)-1)
    {
        perror ("iconv");
    }

    /* 存放转换后的字符串 */
    // printf("outbuf=%s\n", outbuf);

    //存放转换后outbuf剩余的空间
    // printf("outlen=%d\n", (int)*outlen);

    // size_t i = 0;

    // for (i=0; i<(outsize- (*outlen)); i++)
    // {
        // printf("%x\n", outbuf[i]);
    // }

    /* 关闭句柄 */
    iconv_close (cd);

    return 0;
}

#ifdef MAIN
#include <fstream>
int main ()
{
    /* 需要转换的字符串 */
    //char inbuf[1024] = "abcdef哈哈哈哈行";
    const char *text = "你好";

    char inbuf[1024] = { 0 };
    strcpy (inbuf, text);
    size_t inlen = strlen (inbuf);

    /* 存放转换后的字符串 */
    char outbuf[1024] = {};
    size_t outlen = 1024;

    utf8_to_unicode (inbuf, &inlen, outbuf, &outlen);
    printf ("print outbuf: %s\n", outbuf);
    printf("---------------------------------------------------------\n");

    printf ("print inbuf: %s\n", outbuf);
    size_t outsize = strlen(outbuf);
    size_t insize = 1024;
    char instr[1024] = { 0 };
    unicode_to_utf8 (outbuf, &outsize, instr, &insize);
    printf ("print buf: %s\n", instr);
    printf("---------------------------------------------------------\n");

    char buf[1024] = "/home/john/芦苇/linux\\u5BA2\\u6237\\u7AEFt\\u5B89\\u88C5\\u5347\\u7EA7\\u6D4B\\u8BD5\\u6587\\u6863.docx";
    std::string res;
    res = unicode_escape_string_to_utf8_via_u32(buf);
    printf("unicode_escape_string_to_utf8_via_u32<%s> -> <%s>\n", buf, res.c_str());
    if(res.size())
    {
        std::ofstream ofs(res, std::ofstream::app);
        if(ofs)
        {
            ofs << res;
        }
        else
        {
            printf("%s open failed 32\n", res.size());
        }
    }
    res = unicode_escape_string_to_utf8_via_u16(buf);
    printf("unicode_escape_string_to_utf8_via_u16<%s> -> <%s>\n", buf, res.c_str());
    if(res.size())
    {
        std::ofstream ofs(res, std::ofstream::app);
        if(ofs)
        {
            ofs << res;
        }
        else
        {
            printf("%s open failed 16\n", res.size());
        }
    }
    res = unicode_escape_string_to_utf8_via_wchar(buf);
    printf("unicode_escape_string_to_utf8_via_wchar<%s> -> <%s>\n", buf, res.c_str());
    if(res.size())
    {
        std::ofstream ofs(res, std::ofstream::app);
        if(ofs)
        {
            ofs << res;
        }
        else
        {
            printf("%s open failed 16\n", res.size());
        }
    }    return 0;
}
#endif /* MAIN */
