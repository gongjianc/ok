/* Based on src/http/ngx_http_parse.c from NGINX copyright Igor Sysoev
 *
 * Additional changes are licensed under the same terms as NGINX and
 * copyright Joyent, Inc. and other Node contributors. All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

/* Dump what the parser finds to stdout as it happen */

#ifndef PARSERTRACE_H_
#define PARSERTRACE_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <uuid/uuid.h>
#include <pcre.h>
//#include "session_buf.h"
#include "ring_buffer.h"

//typedef int (*http_sbuffputdata) (int iringbuffer_ID, SESSION_BUFFER_NODE ft);

//////////////////////////////////////////////////////////////////////////////////
char Char2Num(char ch);



int http_mch(testhttp_string_and_len ** var, testhttp_string_and_len ** Sto,const char *pattern);



int URLDecode(const char* str, const int strSize, char* result, const int resultSize); 

//////////////////////////////////////////////////////////////////////////////////



int on_message_begin(http_parser* _); 

int on_headers_complete(http_parser* _); 

int on_message_complete(http_parser* _); 

int on_url(http_parser*parser, const char* at, size_t length); 

int on_header_field(http_parser* parser, const char* at, size_t length); 
testhttp_string_and_len *init_testhttp_string_and_len();

int on_header_value(http_parser*parser, const char* at, size_t length); 

int on_body(http_parser*parser, const char* at, size_t length); 

void usage(const char* name); 


int http( HttpSession *p_session);



//int do_http_file(const char *packet_content, int len, struct tuple4 *key)
/* callback function */
//int do_http_file(const char *packet_content, int len, struct tuple4 *key, http_sbuffputdata sbuff_putdata,const int iringbuffer_ID, SESSION_BUFFER_NODE ft);

#endif
