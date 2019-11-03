#include <stdio.h>
#include <string.h>
#include <windows.h>
#include "tinyxml/tinyxml.h"
#include "VM.h"

/*
filter-response.xml
<xml version="1.0" encoding="utf-8">
<document>
	<config console="true | false" />
	<item from="str1" to="str2" type="string | script" >
		... replace strings here
	</item>
	items here...
<document>
*/

enum MatchUrlType {
	MUT_CONTAINS = 0,
	MUT_BEGIN_WITH = 1,
	MUT_END_WITH = 2,
	MUT_MATCH_FULL = MUT_BEGIN_WITH + MUT_END_WITH
};

struct ReplaceItem {
	char *mUrl;
	char *mFrom;
	char *mTo;
	char *mReplace;
	MatchUrlType mMUT;
};

char XML_FILE_PATH[MAX_PATH];
ReplaceItem RIS[100];
int RIS_NUM = 0;
bool EMABLE_CONSOLE = false;

char *Dup(const char *str, bool defEmpty = false) {
	static char EMTYP[4] = {0};
	if (str == NULL) {
		return defEmpty ? EMTYP : NULL;
	}
	int len = strlen(str) + 1;
	char *p = (char *)malloc(len);
	strcpy(p, str);
	return p;
}

void LoadItem(TiXmlElement *item) {
	const char *from = item->Attribute("from");
	const char *to = item->Attribute("to");
	const char *type = item->Attribute("type");
	const char *txt = item->GetText();
	const char *url = item->Attribute("url");

	if (from == NULL || url == NULL || strlen(from) == 0 || strlen(url) == 0) {
		return;
	}

	int mut = MUT_CONTAINS;
	if (*url == '^') {
		mut |= MUT_BEGIN_WITH;
		++url;
	}
	RIS[RIS_NUM].mUrl = Dup(url);
	int urlLen = strlen(RIS[RIS_NUM].mUrl);
	if (urlLen > 0 && RIS[RIS_NUM].mUrl[urlLen - 1] == '$') {
		RIS[RIS_NUM].mUrl[urlLen - 1] = 0;
		mut |= MUT_END_WITH;
	}
	RIS[RIS_NUM].mMUT = MatchUrlType(mut);
	RIS[RIS_NUM].mFrom = Dup(from);
	RIS[RIS_NUM].mTo = Dup(to);
	
	if (type != NULL && strcmp(type, "script") == 0 && txt != NULL) {
		VM vm;
		char *buf = (char *)malloc(200 + strlen(txt));
		buf[0] = 0;
		sprintf(buf, "set -s from = \"%s\" \n set -s to = \"%s\" \n ", from, (to == NULL ? "" : to));
		strcat(buf, txt);
		vm.setContent(from, buf);
		if (! vm.run()) {
			printf("Script Error: %s : %s \n", from, vm.getError());
			return;
		}
		Var *v = vm.getVar("result");
		if (v != NULL) {
			RIS[RIS_NUM].mReplace = Dup(v->mStrVal.c_str(), true);
		} else {
			RIS[RIS_NUM].mReplace = Dup(NULL, true);
		}
	} else {
		char *cnt = Dup(txt, true);
		RIS[RIS_NUM].mReplace = cnt;
	}

	printf("filter[%d]: \n", RIS_NUM);
	printf("    url=[%s]\n", RIS[RIS_NUM].mUrl);
	printf("    from=[%s]\n", RIS[RIS_NUM].mFrom);
	printf("    end=[%s]\n", RIS[RIS_NUM].mTo);
	printf("    replace=[%s]\n", RIS[RIS_NUM].mReplace);

	++RIS_NUM;
}

void LoadFilterResponseFile() {
	TiXmlDocument doc;
	bool loadOkay = doc.LoadFile(XML_FILE_PATH);
	if (! loadOkay) {
		printf( "Could not load xml file: %s. Error='%s' \n", XML_FILE_PATH, doc.ErrorDesc() );
		return;
	}
	TiXmlElement *rootElem = NULL, *node = NULL;
	rootElem = doc.FirstChildElement();
	node = rootElem->FirstChildElement();
	while (node != NULL) {
		const char *nodeName = node->Value();
		if (strcmp(nodeName, "config") == 0) {
			const char *val = node->Attribute("console");
			if (val != NULL && strcmp(val, "true") == 0) {
				EMABLE_CONSOLE = true;
				AllocConsole();
				freopen("CONOUT$", "wb", stdout);
			}
		} else if (strcmp(nodeName, "item") == 0) {
			LoadItem(node);
		}
		node = node->NextSiblingElement();
	}
}

BOOL APIENTRY DllMain( HMODULE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved) {
	switch (ul_reason_for_call) {
	case DLL_PROCESS_ATTACH: {
		GetModuleFileName(hModule, XML_FILE_PATH, MAX_PATH);
		char *p = strrchr(XML_FILE_PATH, '\\');
		if (p != NULL) {
			p[1] = 0;
			strcat(XML_FILE_PATH, "filter-response.xml");
			LoadFilterResponseFile();
		}
		break;}
	case DLL_THREAD_ATTACH:
		break;
	case DLL_THREAD_DETACH:
		break;
	case DLL_PROCESS_DETACH:
		break;
	}
	return TRUE;
}


extern "C" {
	 __declspec(dllexport) bool FilterResponse(const wchar_t *url, char* data_in, size_t data_in_size, size_t& data_in_read, char* data_out, size_t data_out_size, size_t& data_out_written); 
};

bool MatchUrl(char *url, ReplaceItem *item) {
	char *p = strstr(url, item->mUrl);
	if (p == NULL) {
		return false;
	}
	if (item->mMUT & MUT_BEGIN_WITH) {
		if (p != url) {
			return false;
		}
	}
	if (item->mMUT & MUT_END_WITH) {
		int ilen = strlen(item->mUrl);
		int blen = p - url;
		if (ilen + blen != strlen(url)) {
			return false;
		}
	}
	return true;
}

bool ReplaceBetween(char *url, char* data_in, size_t data_in_size, size_t& data_in_read, char* data_out, size_t data_out_size, size_t& data_out_written) {
	char *last = NULL;
	ReplaceItem *item = NULL;
	for (int i = 0; i < RIS_NUM; ++i) {
		ReplaceItem *ri = &RIS[i];
		if (! MatchUrl(url, ri)) {
			continue;
		}
		char *p = strstr(data_in, ri->mFrom);
		if (p == NULL)
			continue;
		if (last == NULL || last > p) {
			last = p;
			item = ri;
		}
	}
	if (last == NULL || item == NULL) {
		return false;
	}

	int rlen = last - data_in;
	memcpy(data_out, data_in, rlen);
	if (item->mReplace != NULL) {
		strcpy(data_out + rlen, item->mReplace);
	}
	data_out_written = rlen + strlen(item->mReplace);

	last += strlen(item->mFrom);
	if (item->mTo != NULL && strlen(item->mTo) != 0) {
		char *p = strstr(last, item->mTo);
		if (p == NULL) {
			// Error
			printf("Error: not find end tag '%s' \n", item->mTo);
			return false;
		}
		p += strlen(item->mTo);
		last = p;
	}
	printf("Replace: %s --> %s \n", item->mFrom, item->mReplace);

	data_in_read = last - data_in;
	return true;
}

bool IsImageUrl(wchar_t *url) {
	wchar_t *w = wcschr(url, '?');
	char hz[8] = {0};
	if (w != NULL) {
		--w;
	} else {
		w = url + wcslen(url) - 1;
	}
	wchar_t *p = wcsrchr(w - 5, '.');
	if (p == NULL || p > w) {
		return false;
	}
	++p;
	for (int i = 0; p <= w; ++i, ++p) {
		hz[i] = tolower(*p);
	}
	
	return strcmp(hz, "png") == 0 || strcmp(hz, "jpg") == 0 || strcmp(hz, "jpeg") == 0 || 
		strcmp(hz, "gif") == 0 || strcmp(hz, "bmp") == 0;
}

bool FilterResponse( const wchar_t *url, char* data_in, size_t data_in_size, size_t& data_in_read, char* data_out, size_t data_out_size, size_t& data_out_written ) {
	static int num = 0;
	char surl[256] = {0};
	for (int i = 0; i < wcslen(url) && i < sizeof(surl) - 1; ++i) {
		surl[i] = (char)url[i];
	}

	// check is image
	if (IsImageUrl((wchar_t *)url)) {
		// printf("Image: %s \n", surl);
		return false;
	}

	// printf("[%d]  ", ++num);
	// printf("url:%s \n", surl);
	// printf("data_in_size: %d  data_out_size:%d \n", data_in_size, data_out_size);

	bool ok = ReplaceBetween(surl, data_in, data_in_size, data_in_read, data_out, data_out_size, data_out_written);
	if (ok) {
		// printf("data_in_read: %d  data_out_write:%d \n\n", data_in_read, data_out_written);
	}
	
	return ok;
}

