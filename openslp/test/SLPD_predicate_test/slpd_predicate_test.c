
#include <assert.h>

#include <libslpattr.h>

#ifdef USE_PREDICATES

typedef void* SLPDPredicate; 
int SLPDTestPredicate(SLPDPredicate predicate, SLPAttributes attr);
SLPError SLPAttrEvalPred(SLPAttributes slp_attr, SLPDPredicate *predicate, SLPBoolean *result, int recursion_depth); /* It's defined in slpd_predicate.c, but since we don't want to pollute namespaces, we put the prototype here. -- we use this so we don't have to calculate the string length, and so we can play with the recursion depth.*/

int wildcard(const char *pattern, const char *string);



void test_wildcard() {
	int err;

	/* Test that initial tokens aren't ignored. */
	err = wildcard("first*second*third*", "first string second third");
	assert(err == 0);

	/* Check that basic equality works. */
	err = wildcard("first", "first");
	assert(err == 0);

	/* Check leading '*'. */
	err = wildcard("*first", "first");
	assert(err == 0);

	/* Check trailing '*'. */
	err = wildcard("first*", "first");
	assert(err == 0);

	/* Check infix '*'. */
	err = wildcard("first*cat", "first dog cat");
	assert(err == 0);

	/* Check multiple infix '*'. */
	err = wildcard("first*roach*cat", "first roach dog cat");
	assert(err == 0);

	/* Check correct substrings, but not anchored at end. */
	err = wildcard("first*roach*cat", "first roach dog cat cheese");
	assert(err == 1);

	/* Check correct substrings, multiple matches, but anchored at end. */
	err = wildcard("first*roach*cat", "first roach dog cat cheese cat");
	assert(err == 0);

	/* Check single wc. */
	err = wildcard("*", "first roach dog cat cheese cat");
	assert(err == 0);

	/* Check single multiple. */
	err = wildcard("****", "first roach dog cat cheese cat");
	assert(err == 0);

	/* Check substring check. */
	err = wildcard("*ach*", "first roach dog cat cheese cat");
	assert(err == 0);
}


void test_predicate() {
	char *str;
	int ierr;
	SLPAttributes slp_attr;
	SLPError err;
	SLPBoolean result;

	
	/******************** Test int stuff. *********************/
	
	err = SLPAttrAlloc("en", NULL, SLP_FALSE, &slp_attr);
	assert(err == SLP_OK);
	
	SLPAttrSet_int(slp_attr, "int", (long)23, SLP_ADD);
	SLPAttrSet_int(slp_attr, "int", (long)25, SLP_ADD);
	SLPAttrSet_int(slp_attr, "int", (long)27, SLP_ADD);
	
	/* Test equals. */
	str = "(&(&(int=23)(int=25))(int=26))";
	ierr = SLPDTestPredicate( str, slp_attr);
	assert(ierr > 0); /* False. */
	
	str = "(&(&(int=24)(int=25))(int=26))";
	ierr = SLPDTestPredicate( str, slp_attr);
	assert(ierr > 0); /* False. */
	
	str = "(&(&(int=24)(int=28))(int=26))";
	ierr = SLPDTestPredicate( str, slp_attr);
	assert(ierr > 0); /* False. */

	str = "(&(&(int=23)(int=25))(int=27))";
	ierr = SLPDTestPredicate( str, slp_attr);
	assert(ierr == 0); /* True. */


	/* Test greater. */
	str = "(int>=29)";
	ierr = SLPDTestPredicate( str, slp_attr);
	assert(ierr > 0); /* f. */
	
	str = "(int>=26)";
	ierr = SLPDTestPredicate( str, slp_attr);
	assert(ierr == 0); /* T. */
	
	str = "(int>=24)";
	ierr = SLPDTestPredicate( str, slp_attr);
	assert(ierr == 0); /* t. */
	
	str = "(int>=22)";
	ierr = SLPDTestPredicate( str, slp_attr);
	assert(ierr == 0); /* t. */

	
	/* Test lesser. */
	str = "(int<=22)";
	ierr = SLPDTestPredicate( str, slp_attr);
	assert(ierr > 0); /* f */
	
	str = "(int<=23)";
	ierr = SLPDTestPredicate( str, slp_attr);
	assert(ierr == 0); /* t */
	
	SLPAttrFree(slp_attr);

	/* Simple equality. */
	err = SLPAttrAllocStr("en", NULL, SLP_FALSE, &slp_attr, "(a=1)");
	assert(err == SLP_OK);

	str = "(a=1)";
	ierr = SLPDTestPredicate( str, slp_attr);
	assert(ierr == 0); /* t */
	
	SLPAttrFree(slp_attr);
	/******************** Test string stuff. *********************/
	
	err = SLPAttrAlloc("en", NULL, SLP_FALSE, &slp_attr);
	assert(err == SLP_OK);
	
	err = SLPAttrSet_str(slp_attr, "str", "string", SLP_REPLACE);
	assert(err == SLP_OK);


	/* Test less (single-valued). */
	str = "(str<=a)";
	ierr = SLPDTestPredicate( str, slp_attr);
	assert(ierr > 0); /* f */
	
	str = "(str<=string)";
	ierr = SLPDTestPredicate( str, slp_attr);
	assert(ierr == 0); /* t */
	
	str = "(str<=strinx)";
	ierr = SLPDTestPredicate( str, slp_attr);
	assert(ierr == 0); /* t */


	/* Test greater (single-valued). */
	str = "(str>=a)";
	ierr = SLPDTestPredicate( str, slp_attr);
	assert(ierr == 0); /* t */
	
	str = "(str>=string)";
	ierr = SLPDTestPredicate( str, slp_attr);
	assert(ierr == 0); /* t */
	
	str = "(str>=strinx)";
	ierr = SLPDTestPredicate( str, slp_attr);
	assert(ierr > 0); /* f */


	/* Test equal (single valued). */
	str = "(str=a)";
	ierr = SLPDTestPredicate( str, slp_attr);
	assert(ierr > 0); /* f */
	
	str = "(str=*ing)";
	ierr = SLPDTestPredicate( str, slp_attr);
	assert(ierr == 0); /* t */
	
	str = "(str=stri*)";
	ierr = SLPDTestPredicate( str, slp_attr);
	assert(ierr == 0); /* t */
	
	str = "(str=*tri*)";
	ierr = SLPDTestPredicate( str, slp_attr);
	assert(ierr == 0); /* t */


	/* TODO Test escaped '*'s. */
	/* TODO Test multivalued. */
	
	SLPAttrFree(slp_attr);
	
	/******************** Test boolean stuff. *********************/
	
	err = SLPAttrAlloc("en", NULL, SLP_FALSE, &slp_attr);
	assert(err == SLP_OK);
	
	err = SLPAttrSet_bool(slp_attr, "bool", SLP_TRUE);
	assert(err == SLP_OK);
	

	/* Test equal. */
	str = "(bool=true)";
	ierr = SLPDTestPredicate( str, slp_attr);
	assert(ierr == 0); /* t */

	str = "(bool=false)";
	ierr = SLPDTestPredicate( str, slp_attr);
	assert(ierr > 0); /* f */

	/* Test bad strings. */
	str = "(bool=falsew)";
	ierr = SLPDTestPredicate( str, slp_attr);
	assert(ierr > 0); /* f */

	str = "(bool=*false)";
	ierr = SLPDTestPredicate( str, slp_attr);
	assert(ierr > 0); /* f */

	str = "(bool=truee)";
	ierr = SLPDTestPredicate( str, slp_attr);
	assert(ierr > 0); /* f */

	str = "(bool= true)";
	ierr = SLPDTestPredicate( str, slp_attr);
	assert(ierr > 0); /* f */

	
	SLPAttrFree(slp_attr);
	
	
	/******************** Test keyword stuff. *********************/
	
	err = SLPAttrAlloc("en", NULL, SLP_FALSE, &slp_attr);
	assert(err == SLP_OK);
	
	err = SLPAttrSet_keyw(slp_attr, "keyw");
	assert(err == SLP_OK);

	/* Test present. */
	str = "(keyw=*)";
	ierr = SLPDTestPredicate( str, slp_attr);
	assert(ierr == 0); /* t */

	str = "(keyw=sd)";
	ierr = SLPDTestPredicate( str, slp_attr);
	assert(ierr > 0); /* f */

	str = "(keyw<=adf)";
	ierr = SLPDTestPredicate( str, slp_attr);
	assert(ierr > 0); /* f */

	SLPAttrFree(slp_attr);

	/********************* Test boolean operators. *********************/
	err = SLPAttrAlloc("en", NULL, SLP_FALSE, &slp_attr);
	assert(err == SLP_OK);
	
	err = SLPAttrSet_keyw(slp_attr, "keyw");
	assert(err == SLP_OK);


	str = "(keyw=*)";
	ierr = SLPDTestPredicate( str, slp_attr);
	assert(ierr == 0); /* t */

	/* Test not. */
	str = "(!(keyw=*))";
	ierr = SLPDTestPredicate( str, slp_attr);
	assert(ierr > 0); /* f */

	str = "(!(!(keyw=*)))";
	ierr = SLPDTestPredicate( str, slp_attr);
	assert(ierr == 0); /* t */

	str = "(!(!(!(keyw=*))))";
	ierr = SLPDTestPredicate( str, slp_attr);
	assert(ierr > 0); /* f */

	str = "(!(!(!(!(keyw=*)))))";
	ierr = SLPDTestPredicate( str, slp_attr);
	assert(ierr == 0); /* t */

	/* Build up to testing binary ops. */
	err = SLPAttrSet_bool(slp_attr, "bool", SLP_TRUE);
	assert(err == SLP_OK);
	
	/* Test and. */
	str = "(&(keyw=*)(bool=true))";
	ierr = SLPDTestPredicate( str, slp_attr);
	assert(ierr == 0); /* t */

	str = "(&(keyw=*)(bool=false))";
	ierr = SLPDTestPredicate( str, slp_attr);
	assert(ierr > 0); /* f */

	str = "(&(keyw=*)(!(bool=false)))";
	ierr = SLPDTestPredicate( str, slp_attr);
	assert(ierr == 0); /* t */

	str = "(&(keywx=*)(bool=true))";
	ierr = SLPDTestPredicate( str, slp_attr);
	assert(ierr > 0); /* f */

	str = "(&(!(keywx=*))(bool=true))";
	ierr = SLPDTestPredicate( str, slp_attr);
	assert(ierr == 0); /* t */

	str = "(&(lkeyw=*)(bool=false))";
	ierr = SLPDTestPredicate( str, slp_attr);
	assert(ierr > 0); /* f */

	str = "(&(!(lkeyw=*))(!(bool=false)))";
	ierr = SLPDTestPredicate( str, slp_attr);
	assert(ierr == 0); /* t */

	str = "(&(&(keyw=*)(bool=true))(&(keyw=*)(bool=true)))";
	ierr = SLPDTestPredicate( str, slp_attr);
	assert(ierr == 0); /* t */

	str = "(&(&(!(keyw=*))(bool=true))(&(keyw=*)(bool=true)))";
	ierr = SLPDTestPredicate( str, slp_attr);
	assert(ierr > 0); /* f */

	str = "(!(&(&(!(keyw=*))(bool=true))(&(keyw=*)(bool=true))))";
	ierr = SLPDTestPredicate( str, slp_attr);
	assert(ierr == 0); /* t */

	
	/* Test sytax errors. */

	/* No preceeding bracket. */
	str = "asdf=log";
	ierr = SLPDTestPredicate( str, slp_attr);
	assert(ierr < 0);

	/* No trailing bracket. */
	str = "(asdf=log";
	ierr = SLPDTestPredicate( str, slp_attr);
	assert(ierr < 0);

	/* Unbalanced brackets. */
	str = "(asdf=log))";
	ierr = SLPDTestPredicate( str, slp_attr);
	assert(ierr < 0);

	str = "((asdf=log)";
	ierr = SLPDTestPredicate( str, slp_attr);
	assert(ierr < 0);

	/* Missing operators. */
	str = "(asdflog)";
	ierr = SLPDTestPredicate( str, slp_attr);
	assert(ierr < 0);

	/* Check that the leaf operator isn't causing the problem. */
	str = "(asdflog=q)";
	ierr = SLPDTestPredicate( str, slp_attr);
	assert(ierr > 0);

	/* Missing logical unary. */
	str = "((asdflog=q))";
	ierr = SLPDTestPredicate( str, slp_attr);
	assert(ierr < 0);

	/* Missing logical binary. */
	str = "((asdflog=q)(asdflog=q))";
	ierr = SLPDTestPredicate( str, slp_attr);
	assert(ierr < 0);

	/* Missing operands and operator. */
	str = "()";
	ierr = SLPDTestPredicate( str, slp_attr);
	assert(ierr < 0);
	
	/* Missing unary operands. */
	str = "(!)";
	ierr = SLPDTestPredicate( str, slp_attr);
	assert(ierr < 0);
	
	/* Missing binary operands. */
	str = "(&)";
	ierr = SLPDTestPredicate( str, slp_attr);
	assert(ierr < 0);
	
	/* Missing binary operands. */
	str = "(=)";
	ierr = SLPDTestPredicate( str, slp_attr);
	assert(ierr < 0);
	
	/* Missing binary operands. I _guess_ this is legal... */
	str = "(thingy=)";
	ierr = SLPDTestPredicate( str, slp_attr);
	assert(ierr > -1);

	/* Trailing trash. */
	str = "(&(a=b)(c=d))";
	ierr = SLPDTestPredicate( str, slp_attr);
	assert(ierr > -1);

	/* Check that the following test will not be short circuited. */
	str = "(a=b)";
	ierr = SLPDTestPredicate( str, slp_attr);
	assert(ierr > 0); /* f */
	
	str = "(|(a=b)(c=d)w)";
	ierr = SLPDTestPredicate( str, slp_attr);
	assert(ierr < 0);

	/* Check recursion depth. */
	str = "(a=b)";
	ierr = SLPAttrEvalPred(slp_attr, (SLPDPredicate)str, &result, 1);
	assert(ierr == 0); /* f */
	
	str = "(!(a=b))";
	ierr = SLPAttrEvalPred(slp_attr, (SLPDPredicate)str, &result, 1);
	assert(ierr < 0);

	/* Check that short-circuiting works. */
	str = "(&(a=b)(!(a=b)))";
	ierr = SLPAttrEvalPred(slp_attr, (SLPDPredicate)str, &result, 2);
	assert(ierr == 0); /* f */

	str = "(|(a=b)(!(a=b)))";
	ierr = SLPAttrEvalPred(slp_attr, (SLPDPredicate)str, &result, 2);
	assert(ierr < 0);

	str = "(|(keyw=*)(!(a=b)))";
	ierr = SLPAttrEvalPred(slp_attr, (SLPDPredicate)str, &result, 2);
	assert(ierr == 0); /* t */
	
	SLPAttrFree(slp_attr);


	/* Check multiple (more than two) subexpressions. */
	err = SLPAttrAlloc("en", NULL, SLP_FALSE, &slp_attr);
	assert(err == SLP_OK);

	err = SLPAttrSet_int(slp_attr, "x", 1, SLP_ADD);
	assert(err == SLP_OK);
	
	str = "(&(x=1)(!(x=1)))";
	ierr = SLPDTestPredicate((SLPDPredicate)str, slp_attr);
	assert(ierr > 0); /* f */

	str = "(&(x=1))";
	ierr = SLPDTestPredicate((SLPDPredicate)str, slp_attr);
	assert(ierr == 0); /* t */

	str = "(&(x=1)(x=1)(x=1))";
	ierr = SLPDTestPredicate((SLPDPredicate)str, slp_attr);
	assert(ierr == 0); /* t */

	SLPAttrFree(slp_attr);
}

int main(int argc, char *argv[]) {
	test_predicate();
	test_wildcard();

	return 0;
}


#else /* USE_PREDICATES */

int main(int argc, char *argv[]) {
	puts("Predicates disabled. Not testing.");
	return 0;
}

#endif /* USE_PREDICATES */
