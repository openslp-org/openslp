
#include <assert.h>

int wildcard(const char *pattern, const char *string); 

int main(int argc, char *argv[]) {
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
