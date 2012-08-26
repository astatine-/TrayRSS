/* Not really a source file 
*  A kind of overview doc to keep the definition together with the rest of the sources 
* 
*
*

Components:
1. TrayRSS - the UI piece
2. libRSS - the heart of the piece - the HTTP client, the RSS parser

Functionality:

Has a Master RSS feed
This will contain a list of RSS feeds to poll (and present)
The refresh time for the rest of the feeds is contained within the Master RSS data

The Master RSS feed is polled at an interval of 24 hours, to check if there are any new RSS feeds to be added to the polling list

The items in the Master RSS feed are used to create a list of RSS feeds to poll and present
Each of the feeds in the items will be fetched at an interval specified in the Master RSS data

The last new item timestamp (per feed) is preserved. If any items as a result of a poll are newer than this 
timestamp then such items are presented one-by-one as a tray popup/bubble

A click on the bubble leads to the default browser being launched with the link associated with that item

The general behavior is to remain quiet - don't give errors on network, server or some other failure.
If it fails, it will try again. And again. Will be set to startup on login.

In the future:
- Do less frequent polling when the system is running on battery power
- Autoproxy detection

*/

void placeholder (void);
