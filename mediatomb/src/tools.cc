/*  tools.cc - this file is part of MediaTomb.

    Copyright (C) 2005 Gena Batyan <bgeradz@deadlock.dhs.org>,
                       Sergey Bostandzhyan <jin@deadlock.dhs.org>

    MediaTomb is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    MediaTomb is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with MediaTomb; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#ifdef HAVE_CONFIG_H
    #include "autoconfig.h"
#endif

#include "tools.h"
#include <sys/stat.h>
#include <sys/time.h>
#include <errno.h>
#include "md5/md5.h"
#include "file_io_handler.h"
#include "metadata_handler.h"

#define WHITE_SPACE " \t\r\n"

using namespace zmm;

static char *HEX_CHARS = "0123456789abcdef";

Ref<Array<StringBase> > split_string(String str, char sep)
{
    Ref<Array<StringBase> > ret(new Array<StringBase>());
    char *data = str.c_str();
    char *end = data + str.length();
    while (data < end)
    {
        char *pos = index(data, sep);
        if (pos == NULL)
        {
            String part(data);
            ret->append(part);
            data = end;
        }
        else if (pos == data)
        {
            data++;
        }
        else
        {
            String part(data, pos - data);
            ret->append(part);
            data = pos + 1;
        }
    }            
    return ret;
}

String trim_string(String str)
{
    if (str == nil)
        return nil;
    int i;
    int start = 0;
    int end = 0;
    int len = str.length();

    char *buf = str.c_str();

    for (i = 0; i < len; i++)
    {
        if (! index(WHITE_SPACE, buf[i]))
        {
            start = i;
            break;
        }
    }
    if (i >= len)
        return _("");
    for (i = len - 1; i >= start; i--)
    {
        if (! index(WHITE_SPACE, buf[i]))
        {
            end = i + 1;
            break;
        }
    }
    return str.substring(start, end - start);
}

bool check_path(String path, bool needDir)
{
    int ret = 0;
    struct stat statbuf;

    ret = stat(path.c_str(), &statbuf);
    if (ret != 0) return false;

    if ((needDir && (!S_ISDIR(statbuf.st_mode))) ||
       (!needDir && (S_ISDIR(statbuf.st_mode)))) return false;

    return true;
}

void check_path_ex(String path, bool needDir)
{
    int ret = 0;
    struct stat statbuf;

    ret = stat(path.c_str(), &statbuf);
    if (ret != 0)
        throw Exception(path + " : " + strerror(errno));

    if (needDir && (!S_ISDIR(statbuf.st_mode)))
        throw Exception(_("Not a directory: ") + path);
    
    if (!needDir && (S_ISDIR(statbuf.st_mode)))
        throw Exception(_("Not a file: ") + path);

}

bool string_ok(String str)
{
    if ((str == nil) || (str == ""))
        return false;

    return true;
}

void string_ok_ex(String str)
{
    if ((str == nil) || (str == ""))
        throw Exception(_("Empty string"));
}

String http_redirect_to(String ip, String port, String page)
{
    return _("<html><head><meta http-equiv=\"Refresh\" content=\"0;URL=http://") + ip + ":" + port + "/" + page + "\"></head><body bgcolor=\"#408bff\"></body></html>";
}

String hex_encode(void *data, int len)
{
    unsigned char *chars;
    int i;
    unsigned char hi, lo;

    Ref<StringBuffer> buf(new StringBuffer(len * 2));
    
    chars = (unsigned char *)data;
    for (i = 0; i < len; i++)
    {
        unsigned char c = chars[i];
        hi = c >> 4;
        lo = c & 0xF;
        *buf << HEX_CHARS[hi] << HEX_CHARS[lo];
    }
    return buf->toString();
    
}

String hex_decode_string(String encoded)
{
	char *ptr = encoded.c_str();
	int len = encoded.length();
	
	Ref<StringBuffer> buf(new StringBuffer(len / 2));
	for (int i = 0; i < len; i += 2)
	{
		char *chi = index(HEX_CHARS, ptr[i]);
		char *clo = index(HEX_CHARS, ptr[i + 1]);
		int hi, lo;
		
		if (chi)
			hi = chi - HEX_CHARS;
		else
			hi = 0;

		if (clo)
			lo = clo - HEX_CHARS;
		else
			lo = 0;
		char ch = (char)(hi << 4 | lo);
		*buf << ch;
	}
	return buf->toString();
}

struct randomizer
{
    struct timeval tv;
    int salt;
};
String hex_md5(void *data, int length)
{
    char md5buf[16];

    md5_state_t ctx;
    md5_init(&ctx);
    md5_append(&ctx, (unsigned char *)data, length);
    md5_finish(&ctx, (unsigned char *)md5buf);

    return hex_encode(md5buf, 16);
}
String hex_string_md5(String str)
{
    return hex_md5(str.c_str(), str.length());
}
String generate_random_id()
{
    struct randomizer st;
    gettimeofday(&st.tv, NULL);
    st.salt = rand();
    return hex_md5(&st, sizeof(st));
}


static const char *hex = "0123456789ABCDEF";

String url_escape(String str)
{
    char *data = str.c_str();
    int len = str.length();
    Ref<StringBuffer> buf(new StringBuffer(len));
    for (int i = 0; i < len; i++)
    {
        unsigned char c = (unsigned char)data[i];
        if ((c >= '0' && c <= '9') ||
            (c >= 'A' && c <= 'Z') ||
            (c >= 'a' && c <= 'z') ||
            c == '_' ||
            c == '-')
        {
            *buf << (char)c;
        }
        else
        {
            int hi = c >> 4;
            int lo = c & 15;
            *buf << '%' << hex[hi] << hex[lo];
        }
    }
    return buf->toString();
}

String url_unescape(String str)
{
    char *data = str.c_str();
    int len = str.length();
    Ref<StringBuffer> buf(new StringBuffer(len));

    int i = 0;
    while (i < len)
    {
        char c = data[i++];
        if (c == '%')
        {
            if (i + 2 > len)
                break; // avoid buffer overrun
            char chi = data[i++];
            char clo = data[i++];
            int hi, lo;

            char *pos;

            pos = index(hex, chi);
            if (!pos)
                hi = 0;
            else
                hi = pos - hex;

            pos = index(hex, clo);
            if (!pos)
                lo = 0;
            else
                lo = pos - hex;

            int ascii = (hi << 4) | lo;
            *buf << (char)ascii;
        }
        else if (c == '+')
        {
            *buf << ' ';
        }
        else
        {
            *buf << c;
        }
    }
    return buf->toString();
}

String mime_types_to_CSV(Ref<Array<StringBase> > mimeTypes)
{
    Ref<StringBuffer> buf(new StringBuffer());
    for (int i = 0; i < mimeTypes->size(); i++)
    {
        if (i > 0)
            *buf << ",";
        String mimeType = mimeTypes->get(i);
        *buf << "http-get:*:" << mimeType << ":*";
    }

    return buf->toString();
}

String read_text_file(String path)
{
	FILE *f = fopen(path.c_str(), "r");
	if (!f)
    {
        throw Exception(_("read_text_file: could not open ") +
                        path + " : " + strerror(errno));
    }
	Ref<StringBuffer> buf(new StringBuffer()); 
	char *buffer = (char *)MALLOC(1024);
	int bytesRead;	
	while((bytesRead = fread(buffer, 1, 1024, f)) > 0)
	{
		*buf << String(buffer, bytesRead);
	}
	fclose(f);
	FREE(buffer);
	return buf->toString();
}
void write_text_file(String path, String contents)
{
    int bytesWritten;
    FILE *f = fopen(path.c_str(), "w");
    if (!f)
    {
        throw Exception(_("write_text_file: could not open ") +
                        path + " : " + strerror(errno));
    }
    
    bytesWritten = fwrite(contents.c_str(), 1, contents.length(), f);
    if (bytesWritten < contents.length())
    {
        fclose(f);
        if (bytesWritten >= 0)
            throw Exception(_("write_text_file: incomplete write to ") +
                            path + " : ");
        else
            throw Exception(_("write_text_file: error writing to ") +
                            path + " : " + strerror(errno));
    }
    fclose(f);
}


/* sorting */
int StringBaseComparator(void *arg1, void *arg2)
{
	return strcmp(((StringBase *)arg1)->data, ((StringBase *)arg2)->data); 
}

static void quicksort_impl(COMPARABLE *a, int lo0, int hi0, COMPARATOR comparator)
{
	int lo = lo0;
	int hi = hi0;

	if (lo >= hi)
	    return;
    if( lo == hi - 1 )
	{
		// sort a two element list by swapping if necessary 
		// if (a[lo] > a[hi])
		if (comparator(a[lo], a[hi]) > 0)
		{
			COMPARABLE T = a[lo];
			a[lo] = a[hi];
			a[hi] = T;
		}
		return;
	}

	// Pick a pivot and move it out of the way
	COMPARABLE pivot = a[(lo + hi) / 2];
	a[(lo + hi) / 2] = a[hi];
	a[hi] = pivot;

	while( lo < hi )
	{
		/* Search forward from a[lo] until an element is found that
		   is greater than the pivot or lo >= hi */ 
		// while (a[lo] <= pivot && lo < hi)
		while (comparator(a[lo], pivot) <= 0 && lo < hi)
		{
			lo++;
		}

		/* Search backward from a[hi] until element is found that
		   is less than the pivot, or lo >= hi */
		// while (pivot <= a[hi] && lo < hi )
		while (comparator(pivot, a[hi]) <= 0 && lo < hi)
		{
			hi--;
	    }

		/* Swap elements a[lo] and a[hi] */
		if( lo < hi )
		{
			COMPARABLE T = a[lo];
			a[lo] = a[hi];
			a[hi] = T;
		}
	}

	/* Put the median in the "center" of the list */
	a[hi0] = a[hi];
	a[hi] = pivot;

	/*
	 *  Recursive calls, elements a[lo0] to a[lo-1] are less than or
	 *  equal to pivot, elements a[hi+1] to a[hi0] are greater than
	 *  pivot.
	 */
	quicksort_impl(a, lo0, lo-1, comparator);
	quicksort_impl(a, hi+1, hi0, comparator);
}

void quicksort(COMPARABLE *arr, int size, COMPARATOR comparator)
{
	quicksort_impl(arr, 0, size - 1, comparator);
}

String renderProtocolInfo(String mimetype, String protocol)
{
    if (string_ok(mimetype) && string_ok(protocol))
        return protocol + ":*:" + mimetype + ":*";
    else
        return _("http-get:*:*:*");
}


String secondsToHMS(int seconds)
{
    int h, m, s;
    
    s = seconds % 60;
    seconds /= 60;

    m = seconds % 60;
    h = seconds / 60;

    // XXX:XX:XX
    char *str = (char *)malloc(10);
    sprintf(str, "%02d:%02d:%02d\n", h, m, s);
    return String::take(str);
}

#ifdef HAVE_MAGIC
String get_mime_type(magic_set *ms, Ref<RExp> reMimetype, String file)
{
    if (ms == NULL)
        return nil;

    char *mt = (char *)magic_file(ms, file.c_str());
    if (mt == NULL)
    {
        log_error("magic_file: %s\n", magic_error(ms));
        return nil;
    }

    String mime_type(mt);

    Ref<Matcher> matcher = reMimetype->matcher(mime_type, 2);
    if (matcher->next())
        return matcher->group(1);

    log_warning("filemagic returned invalid mimetype for %s\n%s\n",
                file.c_str(), mt);
    return nil;
}

#endif 

void set_jpeg_resolution_resource(Ref<CdsItem> item, int res_num)
{
    try
    {
        Ref<IOHandler> fio_h(new FileIOHandler(item->getLocation()));
        fio_h->open(UPNP_READ);
        String resolution = get_jpeg_resolution(fio_h);

        if (res_num >= item->getResourceCount())
            throw Exception(_("Invalid resource index"));
            
        item->getResource(res_num)->addAttribute(MetadataHandler::getResAttrName(R_RESOLUTION), resolution);
    }
    catch (Exception e)
    {
        e.printStackTrace();
    }
}

