/*
	OpenLieroX

	UTF8/Unicode conversions
	
	code under LGPL
	created 01-05-2007
	by Albert Zeyer and Dark Charlie
*/

#ifndef __UNICODE_H__
#define __UNICODE_H__

#include <SDL/SDL.h> // for Uint32
#include "UCString.h"

#include "Utils.h"


typedef Uint32 UnicodeChar;
// the iterator shows at the next character after this operation
UnicodeChar GetNextUnicodeFromUtf8(UCString::const_iterator& it, const UCString::const_iterator& last);
UCString GetUtf8FromUnicode(UnicodeChar ch);



template<typename _Iterator>
void IncUtf8StringIterator(_Iterator& it, const _Iterator& last) {
	unsigned char c;
	for(it++; it != last; it++) {
		c = *it;
		if(!(c&0x80) || (c&0xC0)) break;
	}
}

template<typename _Iterator>
void MultIncUtf8StringIterator(_Iterator& it, const _Iterator& last, size_t count) {
	for(size_t i = 0; i < count; i++) {
		if(it == last) break;
		IncUtf8StringIterator(it, last);
	}
}

inline size_t Utf8StringLength(const UCString& utf8string) {
	size_t len = 0;
	UCString::const_iterator it = utf8string.begin();
	while(it != utf8string.end()) {
		len++;
		IncUtf8StringIterator(it, utf8string.end());
	}
	return len;
}

// returns the new pos
inline size_t InsertUnicodeChar(UCString& str, size_t pos, UnicodeChar ch) {
	UCString tmp = GetUtf8FromUnicode(ch);
	str.insert(pos, tmp);
	return pos + tmp.size();
}

#endif
