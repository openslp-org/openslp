
#ifndef SLP_ATTR_H_INCLUDED
#define SLP_ATTR_H_INCLUDED

//#include <slp.h>
#include "../libslp/slp.h"
#include <stdio.h>

#define SLP_TAG_BAD 300
#define SLP_TAG_ERROR 400

/* The type for SLP attributes. An opaque type that acts as a handle to an 
 * attribute bundle.
 */
typedef void *SLPAttributes;
typedef void *SLPAttrIterator;
typedef void *SLPTemplate;


/* The callback for receiving attributes from a SLPFindAttrObj(). */ 
typedef SLPBoolean SLPAttrObjCallback( 
	SLPHandle hslp, 
	const SLPAttributes attr, 
	SLPError errcode, 
	void* cookie 
);

/* A datatype to encapsulate opaque data.*/
typedef struct {
	size_t len;
	char *data;
} SLPOpaque;


/*****************************************************************************
 *
 * Datatype to represent the types of known attributes. 
 * 
 ****************************************************************************/
typedef int SLPType;
#define SLP_BOOLEAN ((SLPType)1)
#define SLP_INTEGER ((SLPType)2)
#define SLP_KEYWORD ((SLPType)4)
#define SLP_STRING ((SLPType)8)
#define SLP_OPAQUE ((SLPType)16)


/*****************************************************************************
 *
 * Datatype to represent modes of attribute modification.
 * 
 ****************************************************************************/
typedef enum {
	SLP_ADD = 1, /* Appends to the attribute list.*/
	SLP_REPLACE = 2 /* Replaces attribute. */
} SLPInsertionPolicy;


/*****************************************************************************
 *
 * Functions for the attribute struct. One could almost call them methods. 
 * 
 ****************************************************************************/

SLPError SLPAttrAlloc(
		const char *lang, 
		const FILE *template_h,
		const SLPBoolean strict, 
		SLPAttributes *slp_attr
);

SLPError SLPAttrAllocStr(
		const char *lang, 
		const FILE *template_h,
		const SLPBoolean strict, 
		SLPAttributes *slp_attr,
		const char *str
);

void SLPAttrFree(SLPAttributes attr_h);

/* Attribute manipulation. */
SLPError SLPAttrSet_bool(
		SLPAttributes attr_h,
		const char *attribute_tag,
		SLPBoolean val
);

SLPError SLPAttrSet_str(
		SLPAttributes attr_h,
		const char *tag,
		const char *val,
		SLPInsertionPolicy 
);

SLPError SLPAttrSet_keyw(
		SLPAttributes attr_h,
		const char *attribute_tag
);

SLPError SLPAttrSet_int(
		SLPAttributes attr_h,
		const char *tag,
		long val,
		SLPInsertionPolicy policy
); 

SLPError SLPAttrSet_opaque(
		SLPAttributes attr_h,
		const char *tag,
		const char *val,
		const unsigned int len, 
		SLPInsertionPolicy policy
);

SLPError SLPAttrSet_guess(
		SLPAttributes attr_h,
		const char *tag,
		const char *val,
		SLPInsertionPolicy policy
);


/* Attribute Querying. */
SLPError SLPAttrGet_bool(
		SLPAttributes attr_h,
		const char *tag,
		SLPBoolean *val
);

SLPError SLPAttrGet_keyw(
		SLPAttributes attr_h,
		const char *tag
);

SLPError SLPAttrGet_int(
		SLPAttributes attr_h,
		const char *tag,
		long *val[],
		size_t *size
);

SLPError SLPAttrGet_str(
		SLPAttributes attr_h,
		const char *tag,
		char ***val,
		size_t *size
); 


SLPError SLPAttrGet_opaque(
		SLPAttributes attr_h,
		const char *tag,
		SLPOpaque ***val,
		size_t *size
);


/* Misc. */
SLPError SLPAttrGetType(SLPAttributes attr_h, const char *tag, SLPType *type);

//SLPError SLPAttrSerialize(SLPAttributes attr_h, size_t *count, char **str, SLPBoolean);
SLPError SLPAttrSerialize(SLPAttributes attr_h,
		const char* tags /* NULL terminated */,
		char **buffer,
		size_t bufferlen, /* Size of buffer. */
		size_t* count, /* Bytes needed/written. */
		SLPBoolean find_delta
);

SLPError SLPAttrFreshen(SLPAttributes attr_h, const char *new_attrs);

/* Functions. */
SLPError SLPRegAttr( 
	SLPHandle slp_h, 
	const char* srvurl, 
	unsigned short lifetime, 
	const char* srvtype, 
	SLPAttributes attr_h, 
	SLPBoolean fresh, 
	SLPRegReport callback, 
	void* cookie 
);

SLPError SLPFindAttrObj (
		SLPHandle hslp, 
		const char* srvurlorsrvtype, 
		const char* scopelist, 
		const char* attrids, 
		SLPAttrObjCallback *callback, 
		void* cookie
);
	


/*****************************************************************************
 *
 * Functions for the iterator struct
 * 
 ****************************************************************************/

SLPError SLPAttrIteratorAlloc(SLPAttributes attr, SLPAttrIterator *iter);
void SLPAttrIteratorFree(SLPAttrIterator iter);

SLPBoolean SLPAttrIterNext(SLPAttrIterator iter_h, char const **tag, SLPType *type);


#endif
