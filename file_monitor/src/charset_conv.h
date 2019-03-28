#pragma once

/*
 * convert string [with unicode format] to string [with utf8]
 * params:
 *      1, string like "a\\u4F60\\u597D.txt"
 * retval:
 *      string like "a(0x4F)(0x60)(0x59)(0x7D).txt"
 * egg:
 *      "a\\u4F60\\u597D.txt" -> "a你好.txt"
 */
std::string unicode_escape_string_to_utf8(const std::string source);

/*
 * convert string [with unicode format] to string [with utf8 encoding]
 * params:
 *      1, string like "a\\u4F60\\u597D.txt"
 * retval:
 *      string like "a你好.txt"
 * egg:
 *      "a\\u4F60\\u597D.txt" -> "a你好.txt"
 */
std::string unicode_escape_string_to_utf8_via_u16(const std::string source);
std::string unicode_escape_string_to_utf8_via_u32(const std::string source);
std::string unicode_escape_string_to_utf8_via_wchar(const std::string source);


/*
 * convert string [with ucs-2 encoding] and string [with utf8 encoding] each other
 * params:
 *      string like "你好" and capasity
 *      string like "`O}Y" and capasity
 * egg:
 *      "a\\u4F60\\u597D.txt" -> "a你好.txt"
 */
bool utf8_to_unicode (char *inbuf, size_t *inlen, char *outbuf, size_t *outlen);
bool unicode_to_utf8 (char *inbuf, size_t *inlen, char *outbuf, size_t *outlen);
