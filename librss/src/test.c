#ifdef _TEST

#include "RSS.h"
#include "RSS_Buffer.h"
#include "RSS_Parser.h"
#include "RSS_Http.h"

#ifdef _DEBUG
# define CRTDBG_MAP_ALLOC
# include <stdlib.h>
# include <crtdbg.h>
#endif

#include <stdio.h>

#ifdef RSS_USE_WSTRING
# define RSS_fopen      _wfopen
//# define RSS_sprintf(x, ...) swprintf(x, 255, __VA_ARGS__)
#else
# define RSS_fopen      fopen
//# define RSS_sprintf    sprintf
#endif

void test_error_handler(RSS_u32 error_level, const RSS_char* error, size_t pos)
{
	if(error_level == RSS_EL_ERROR)
	{
		if(pos != RSS_NO_POS_INFO)
			RSS_printf(RSS_text("[%d] %s\n"), pos, error);
		else
			RSS_printf(RSS_text("%s\n"), error);
	}
}

void test_time3339(const RSS_char* str_time)
{
	time_t t;

	t = RSS_parse_RFC3339_Date(str_time, test_error_handler);
	if(t)
		RSS_printf(RSS_text("test_time3339 [OK]:\n%s==?\n%s\n"), RSS_ctime(&t), str_time);
	else
		RSS_printf(RSS_text("test_time3339 [FAIL]: %s\n"), str_time);
}

void test_time822(const RSS_char* str_time)
{
	time_t t;

	t = RSS_parse_RFC822_Date(str_time, test_error_handler);
	if(t)
		RSS_printf(RSS_text("test_time822 [OK]:\n%s==?\n%s\n"), RSS_ctime(&t), str_time);
	else
		RSS_printf(RSS_text("test_time822 [FAIL]: %s\n"), str_time);
}

void test_feed(const RSS_char* url)
{
	RSS_Feed* feed;
	
	feed = RSS_create_feed(url, test_error_handler);
	if(feed)
		RSS_printf(RSS_text("test_feed [OK]: %s\n"), url);
	else
		RSS_printf(RSS_text("test_feed [OK] - wrong format: %s\n"), url);
	RSS_free_feed(feed);
}

void test_feed_from_str(const RSS_char* str)
{
	RSS_Feed* feed;
	
	feed = RSS_create_feed_from_str(str, test_error_handler);
	if(feed)
		RSS_printf(RSS_text("test_feed [OK]\n"));
	else
		RSS_printf(RSS_text("test_feed [OK] - wrong format\n"));
	RSS_free_feed(feed);
}

void test_buffer(void)
{
	RSS_Buffer* buff;
	int i;

	buff = RSS_create_buffer();
	for(i=0; i<1300; i++)
		RSS_add_buffer(buff, RSS_text('m'));

	if(buff->len != 1300 || buff->reserved != (RSS_BUFFER_INITIAL_SIZE*4))
		RSS_printf(RSS_text("test_buffer [FAIL]\n"));
	else
		RSS_printf(RSS_text("test_buffer [OK]\n"));

	RSS_free_buffer(buff);
}

void test_http(RSS_char* http_address)
{
	char*			page;
	RSS_Url*		url;
	RSS_Http_error	res;

	url = RSS_create_url(http_address);
	if(!url)
	{
		RSS_printf(RSS_text("test_http [FAIL]: url not created\n"));
		return;
	}

	if((res = RSS_http_get_page(url, &page)) != RSS_HTTP_OK)
	{
		RSS_printf(RSS_text("test_http [FAIL]: http error: %d\n"), res);
		RSS_free_url(url);
		return;
	}
	
	free(page);
	RSS_free_url(url);
	RSS_printf(RSS_text("test_http [OK]\n"));
}

/* convert time in seconds to an "English" description - e.g. "2 days ago", "47 minutes ago" etc
	Maximum space taken to describe age is 100 characters */
RSS_char *RSS_time2age(double tsince, int szAgeStr, RSS_char *age)
{
	if(tsince < 60) {  // less than 60 seconds
		RSS_sprintf(age, szAgeStr, RSS_text("Less than a minute ago"));
		return age;
	}

	if(tsince < 60*2) {  // less than 120 seconds
		RSS_sprintf(age, szAgeStr, RSS_text("About a minute ago"));
		return age;
	}
	
	if(tsince < 60*60){ // less than 3660 seconds = 1 hour
		RSS_sprintf(age, szAgeStr, RSS_text("About %d minutes ago"), (int)tsince/60);
		return age;
	}

	if(tsince < 2*60*60){ // less than 7260 seconds = 2 hour
		RSS_sprintf(age, szAgeStr, RSS_text("About an hour ago"), (int)tsince/60);
		return age;
	}

	if(tsince < 24*60*60){ // less than 24 hours ago
		RSS_sprintf(age,szAgeStr, RSS_text("%d hours ago"), (int)tsince/(60*60));
		return age;
	}

	if(tsince < 48*60*60){ // less than 48 hours ago
		RSS_sprintf(age,szAgeStr, RSS_text("Yesterday"));
		return age;
	}
	
	if(tsince < 7*24*60*60){ // less than 7 days ago
		RSS_sprintf(age, szAgeStr, RSS_text("This week"));
		return age;
	}

	if(tsince < 2*7*24*60*60){ // less than 14 days 
		RSS_sprintf(age, szAgeStr, RSS_text("Last week"));
		return age;
	}

	if(tsince < 30*24*60*60){ // less than 30 days 
		RSS_sprintf(age, szAgeStr, RSS_text("This month"));
		return age;
	}

	if(tsince < 2*30*24*60*60){ // less than 60 days
		RSS_sprintf(age, szAgeStr, RSS_text("Last month"));
		return age;
	}
	if(tsince < 365*24*60*60){ // less than 365 days 
		RSS_sprintf(age, szAgeStr, RSS_text("This year"));
		return age;
	}

	RSS_sprintf(age, szAgeStr, RSS_text("More than an year ago"));

	return age;
}
void test_feed_walk(const RSS_char* url)
{
	RSS_Feed* feed;
	
	feed = RSS_create_feed(url, test_error_handler);
	if(feed) {
		RSS_Item *fi;
		double tsince;
		time_t tnow;
		int RSS_item_count = 0;
		RSS_char age[101];

		fi = feed->items;
		while (fi) {
			RSS_item_count++;
			tsince = difftime(time(&tnow),fi->pubDate);
			RSS_time2age(tsince,101,age);
			RSS_printf(RSS_text("[%d]Title:\t%s [%s]\n[%d]Link:\t%s\n"),RSS_item_count, fi->title, age, RSS_item_count, fi->link);
			fi = fi->next;
		}
		RSS_printf(RSS_text("Items in feed: %d\n\n"),RSS_item_count);
	}
	else
		RSS_printf(RSS_text("test_feed [OK] - wrong format: %s\n"), url);

	RSS_free_feed(feed);
}


int main(void)
{
	RSS_char	filename[261];
	int			i;
	FILE*		f;
	WSADATA wsaData;
	int iResult;

	
#if defined(_DEBUG) && defined(_MSC_VER)
	_CrtSetDbgFlag ( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );
#endif



// Initialize Winsock
	iResult = WSAStartup(MAKEWORD(2,2), &wsaData);
	if (iResult != 0) {
		printf("WSAStartup failed: %d\n", iResult);
		return 1;
	}


	test_time3339(RSS_text("2011-02-03T09:56:44Z"));
	test_time3339(RSS_text("2011-02-02T18:56:48Z"));
	test_time3339(RSS_text("2011-01-18T11:11:22Z"));
	test_time3339(RSS_text("2011-01-20T09:45:39Z"));

	test_time822(RSS_text("Sun, 06 Feb 2011 15:46:39 GMT"));
	test_time822(RSS_text("Sun, 06 Feb 2011 15:46:39"));

	test_buffer();

	test_feed_walk(RSS_text("feeds.bbci.co.uk/news/rss.xml"));

	test_http(RSS_text("http://feeds.wired.com/wired/index?format=xml"));
	test_http(RSS_text("feeds.bbci.co.uk/news/rss.xml"));
	test_http(RSS_text("http://feeds.bbci.co.uk/news/rss.xml"));
//	test_http(RSS_text("http://forum.k2t.eu"));
//	test_http(RSS_text("forum.k2t.eu/"));
//	test_http(RSS_text("forum.k2t.eu"));

	test_feed(RSS_text("feeds.bbci.co.uk/news/rss.xml"));
	test_feed(RSS_text("http://rss.slashdot.org/Slashdot/slashdot"));
	test_feed(RSS_text("http://www.pinkbike.com/pinkbike_xml_feed.php"));
	test_feed(RSS_text("http://www.filmweb.pl/feed/news/latest"));
	test_feed(RSS_text("http://feeds.feedburner.com/niebezpiecznik/"));



	for(i=1;i<=15;i++) /* max 156615, TODO: 14341 - stack overflow */
	{
		RSS_sprintf(filename, 260, RSS_text("..\\..\\testing_xml\\%d.xml"), i);
		f = RSS_fopen(filename, RSS_text("rb"));
		if(f) 
		{
			RSS_Encoding	encoding;
			char*			encoded;
			long			file_size;
			RSS_char*		decoded;

			fseek(f, 0, SEEK_END);
			file_size = ftell(f);
			fseek(f, 0, SEEK_SET);

			encoded = (char*)malloc(file_size+1);
			fread(encoded, 1, file_size, f);
			fclose(f);
			encoded[file_size] = 0;

			RSS_printf(RSS_text("Testing %d.xml: "), i);
			encoding = RSS_determine_encoding(encoded);
			if(encoding != RSS_ENC_NO_INFO && encoding != RSS_ENC_UNSUPP)
			{
                decoded = char2RSS_str(encoded, encoding);
				test_feed_from_str(decoded);
				free(decoded);
			}
			else
			{
				RSS_printf(RSS_text("test_buffer [FAIL]\n"));
				test_error_handler(RSS_EL_ERROR, RSS_text("Unknown encoding"), RSS_NO_POS_INFO);
			}
			free(encoded);
		} /* if(f) */
		else
            RSS_printf(RSS_text("test_buffer [FAIL], cannot open file %s\n"), filename);
	} /* for(all_xml) */

	WSACleanup();

	return 0;
}

#else

/* To prevent compiler warning */
void dummy(void);

#endif
