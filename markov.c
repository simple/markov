#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>


enum {
	NPREF = 2,	/* 접두사의 단어 수 */
	NHASH = 4093,	/* 상태를 저장할 해시 테이블의 배열 크기 */
	MAXGEN = 10000	/* 생성할 수 있는 최대 단어 수 */
};

typedef struct State State;
typedef struct Suffix Suffix;
struct State {	/* 접두사 + 접미사 리스트 */
	char	*pref[NPREF];	/* 접두사들 */
	Suffix	*suf;			/* 접미사 리스트 */
	State	*next;			/* 해시 테이블 안에서 다음 State */
};
struct Suffix {	/* 접미사 리스트 */
	char	*word;	/* 접미사 */
	Suffix	*next;	/* 접미사 리스트 안에서 다음 Suffix */
};

unsigned int hash(char *s[NPREF]);
State* lookup(char *prefix[NPREF], int create);
void build(char *prefix[NPREF], FILE *f);
void add(char *prefix[NPREF], char *suffix);
void addsuffix(State *sp, char *suffix);
void generate(int nwords, int debug);


State *statetab[NHASH];	/* State Hash Table */
char NOWORD[] = "\n";	/* 일반 단어로 사용하지 못하는 단어 */


/* hash: NPREF 개의 문자열로 구성된 배열의 해시 값 계산 */
unsigned int hash(char *s[NPREF])
{
	enum { MULTIPLIER = 31 };

	unsigned int h;
	unsigned char *p;
	int i;

	h = 0;
	for (i = 0; i < NPREF; i++)
		for (p = (unsigned char *) s[i]; *p != '\0'; p++)
			h = MULTIPLIER * h + *p;
	return h % NHASH;
}

/* lookup: 접두사를 검색, 필요할 경우 접두사를 생성 */
/* 접두사를 찾았거나, 만들었으면 포인터를 리턴, 그 외의 경우에 NULL 리턴 */
/* 새로 만든 문자열은 strdup으로 만든 것이 아니므로 변경하면 안됨 */
State* lookup(char *prefix[NPREF], int create)
{
	int i, h;
	State *sp;

	h = hash(prefix);
	for (sp = statetab[h]; sp != NULL; sp = sp->next) {
		for (i = 0; i < NPREF; i++)
			if (strcmp(prefix[i], sp->pref[i]) != 0)
				break;
		if (i == NPREF)	/* Found it! */
			return sp;
	}

	if (create) {
		sp = (State *) malloc(sizeof(State));
		for (i = 0; i < NPREF; i++)
			sp->pref[i] = prefix[i];
		sp->suf = NULL;
		sp->next = statetab[h];
		statetab[h] = sp;
	}
	return sp;
}

/* build: 입력을 읽고 해시 테이블에 저장 */
void build(char *prefix[NPREF], FILE *f)
{
	char buf[100], fmt[10];

	/* 포멧 문자열 작성, %s만 쓸 경우 buf가 오버플로우할 가능성이 있음 */
	sprintf(fmt, "%%%ds", sizeof(buf)-1);
	while (fscanf(f, fmt, buf) != EOF)
		add(prefix, strdup(buf));
}

/* add: 단어를 접미사 리스트에 추가하고, 접두사를 갱신한다 */
void add(char *prefix[NPREF], char *suffix)
{
	State *sp;
	sp = lookup(prefix, 1);	/* 없으면 생성한다 */
	addsuffix(sp, suffix);
	/* 접두사 배열에서 단어들을 하나씩 앞으로 당긴다 */
	memmove(prefix, prefix+1, (NPREF-1)*sizeof(prefix[0]));
	prefix[NPREF-1] = suffix;
}

/* addsuffix: Suffix를 state에 추가. 이 때 접미사는 나중에 변경하면... ??? */
void addsuffix(State *sp, char *suffix)
{
	Suffix *suf;
	suf = (Suffix *) malloc(sizeof(Suffix));
	suf->word = suffix;
	suf->next = sp->suf;
	sp->suf = suf;
}

/* generate: 한 줄에 한 단어씩 출력 생성 */
void generate(int nwords, int debug)
{
	State *sp;
	Suffix *suf;
	char *prefix[NPREF], *w;
	int i, nmatch;

	srand(time(NULL));

	for (i = 0; i < NPREF; i++)	/* 접두사 초기화 */
		prefix[i] = NOWORD;

	for (i = 0; i < nwords; i++) {
		sp = lookup(prefix, 0);
		nmatch = 0;
		for (suf = sp->suf; suf != NULL; suf = suf->next)
			if (rand() % ++nmatch == 0)	/* 확률 = 1 / nmatch */
				w = suf->word;
		if (strcmp(w, NOWORD) ==  0)
			break;
		if (debug)
			printf("pref[0] = %s, pref[1] = %s, suffix = %s\n", prefix[0], prefix[1], w);
		else
			printf("%s ", w);

		memmove(prefix, prefix+1, (NPREF-1)*sizeof(prefix[0]));
		prefix[NPREF-1] = w;
	}
}

/* markov main: 마르코프 체인을 이용한 텍스트 생성 */
int main(int argc, char *argv[])
{
	int i, nwords = MAXGEN;
	int debug = 0;
	char *prefix[NPREF];	/* 현재 입력 접두사 */

	if (argc == 2 && strcmp(argv[1], "-d") == 0)
		debug = 1;

	for (i = 0; i < NPREF; i++)	/* 최초 접두사를 설정한다 */
		prefix[i] = NOWORD;
	build(prefix, stdin);
	add(prefix, NOWORD);
	generate(nwords, debug);
	return 0;
}
