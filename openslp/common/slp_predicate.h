#include "slp_attr.h"
#include "slp_filter.h"

#ifdef DEBUG
void dumpAttrList(int level, const lslpAttrList *attrs);
void dumpFilterTree(const lslpLDAPFilter *filter);
#endif

int lslp_predicate_match(const lslpAttrList *a, const char *b);

