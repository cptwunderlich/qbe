#include <assert.h>
#include <inttypes.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAKESURE(what, x) typedef char make_sure_##what[(x)?1:-1]
#define die(...) die_(__FILE__, __VA_ARGS__)

typedef unsigned int uint;
typedef unsigned short ushort;
typedef unsigned long ulong;
typedef unsigned long long bits;

typedef struct BSet BSet;
typedef struct Ref Ref;
typedef struct OpDesc OpDesc;
typedef struct Ins Ins;
typedef struct Phi Phi;
typedef struct Blk Blk;
typedef struct Use Use;
typedef struct Tmp Tmp;
typedef struct Con Con;
typedef struct Addr Mem;
typedef struct Fn Fn;
typedef struct Typ Typ;
typedef struct Dat Dat;

enum Reg {
	RXX,

	RAX, /* caller-save */
	RCX,
	RDX,
	RSI,
	RDI,
	R8,
	R9,
	R10,
	R11,

	RBX, /* callee-save */
	R12,
	R13,
	R14,
	R15,

	RBP, /* reserved */
	RSP,

	XMM0, /* sse */
	XMM1,
	XMM2,
	XMM3,
	XMM4,
	XMM5,
	XMM6,
	XMM7,
	XMM8,
	XMM9,
	XMM10,
	XMM11,
	XMM12,
	XMM13,
	XMM14,
	XMM15,

	Tmp0, /* first non-reg temporary */

	NIReg = R15 - RAX + 1,
	NFReg = XMM14 - XMM0 + 1, /* XMM15 is reserved */
	NISave = R11 - RAX + 1,
	NFSave = NFReg,
	NRSave = NISave + NFSave,
	NRClob = R15 - RBX + 1,
};

enum {
	NString = 32,
	NPred   = 63,
	NIns    = 8192,
	NAlign  = 3,
	NSeg    = 32,
	NTyp    = 128,
	NBit    = CHAR_BIT * sizeof(bits),
};

MAKESURE(NBit_is_enough, NBit >= (int)Tmp0);

#define BIT(n) ((bits)1 << (n))

struct BSet {
	uint nt;
	bits *t;
};

struct Ref {
	uint32_t type:3;
	uint32_t val:29;
};

enum Alt {
	AType,
	ACall,
	AMem,

	AShift = 28,
	AMask = (1<<AShift) - 1
};

enum {
	RTmp,
	RCon,
	RSlot,
	RType,
	RCall,
	RMem,
};

#define R        (Ref){0, 0}
#define TMP(x)   (Ref){RTmp, x}
#define CON(x)   (Ref){RCon, x}
#define CON_Z    CON(0)          /* reserved zero constant */
#define SLOT(x)  (Ref){RSlot, x}
#define TYPE(x)  (Ref){RType, x}
#define CALL(x)  (Ref){RCall, x}
#define MEM(x)   (Ref){RMem, x}

static inline int req(Ref a, Ref b)
{
	return a.type == b.type && a.val == b.val;
}

static inline int rtype(Ref r)
{
	if (req(r, R))
		return -1;
	return r.type;
}

static inline int isreg(Ref r)
{
	return rtype(r) == RTmp && r.val < Tmp0;
}

enum ICmp {
#define ICMPS(X) \
	X(ule)   \
	X(ult)   \
	X(sle)   \
	X(slt)   \
	X(sgt)   \
	X(sge)   \
	X(ugt)   \
	X(uge)   \
	X(eq)    \
	X(ne) /* make sure icmpop() below works! */

#define X(c) IC##c,
	ICMPS(X)
#undef X
	NICmp,

	ICxnp = NICmp, /* x64 specific */
	ICxp,
	NXICmp
};

static inline int icmpop(int c)
{
	return c >= ICeq ? c : ICuge - c;
}

enum FCmp {
#define FCMPS(X) \
	X(le)    \
	X(lt)    \
	X(gt)    \
	X(ge)    \
	X(ne)    \
	X(eq)    \
	X(o)     \
	X(uo)

#define X(c) FC##c,
	FCMPS(X)
#undef X
	NFCmp
};

enum Class {
	Kx = -1, /* "top" class (see usecheck() and clsmerge()) */
	Kw,
	Kl,
	Ks,
	Kd
};

#define KWIDE(k) ((k)&1)
#define KBASE(k) ((k)>>1)

enum Op {
	Oxxx,

	/* public instructions */
	Oadd,
	Osub,
	Odiv,
	Orem,
	Oudiv,
	Ourem,
	Omul,
	Oand,
	Oor,
	Oxor,
	Osar,
	Oshr,
	Oshl,
	Ocmpw,
	Ocmpw1 = Ocmpw + NICmp-1,
	Ocmpl,
	Ocmpl1 = Ocmpl + NICmp-1,
	Ocmps,
	Ocmps1 = Ocmps + NFCmp-1,
	Ocmpd,
	Ocmpd1 = Ocmpd + NFCmp-1,

	Ostoreb,
	Ostoreh,
	Ostorew,
	Ostorel,
	Ostores,
	Ostored,
#define isstore(o) (Ostoreb <= o && o <= Ostored)
	Oloadsb,  /* needs to match OExt (mem.c) */
	Oloadub,
	Oloadsh,
	Oloaduh,
	Oloadsw,
	Oloaduw,
	Oload,
#define isload(o) (Oloadsb <= o && o <= Oload)
	Oextsb,
	Oextub,
	Oextsh,
	Oextuh,
	Oextsw,
	Oextuw,
#define isext(o) (Oextsb <= o && o <= Oextuw)

	Oexts,
	Otruncd,
	Ostosi,
	Odtosi,
	Oswtof,
	Osltof,
	Ocast,

	Oalloc,
	Oalloc1 = Oalloc + NAlign-1,

	Ocopy,
	NPubOp,

	/* function instructions */
	Opar = NPubOp,
	Oparc,
	Oarg,
	Oargc,
	Ocall,

	/* reserved instructions */
	Onop,
	Oaddr,
	Oswap,
	Osign,
	Osalloc,
	Oxidiv,
	Oxdiv,
	Oxcmp,
	Oxset,
	Oxsetnp = Oxset + ICxnp,
	Oxsetp  = Oxset + ICxp,
	Oxtest,
	NOp
};

enum Jmp {
	Jxxx,
	Jret0,
	Jretw,
	Jretl,
	Jrets,
	Jretd,
	Jretc,
#define isret(j) (Jret0 <= j && j <= Jretc)
	Jjmp,
	Jjnz,
	Jxjc,
	Jxjnp = Jxjc + ICxnp,
	Jxjp  = Jxjc + ICxp,
	NJmp
};

struct OpDesc {
	char *name;
	int nmem;
	short argcls[2][4];
	uint sflag:1; /* sets the zero flag */
	uint lflag:1; /* leaves flags */
	uint cfold:1; /* can fold */
};

struct Ins {
	uint op:30;
	Ref to;
	Ref arg[2];
	uint cls:2;
};

struct Phi {
	Ref to;
	Ref arg[NPred];
	Blk *blk[NPred];
	uint narg;
	int cls;
	Phi *link;
};

struct Blk {
	Phi *phi;
	Ins *ins;
	uint nins;
	struct {
		short type;
		Ref arg;
	} jmp;
	Blk *s1;
	Blk *s2;
	Blk *link;

	int id;
	int visit;

	Blk *idom;
	Blk *dom, *dlink;
	Blk **fron;
	int nfron;

	Blk **pred;
	uint npred;
	BSet in[1], out[1], gen[1];
	int nlive[2];
	int loop;
	char name[NString];
};

struct Use {
	enum {
		UXXX,
		UPhi,
		UIns,
		UJmp,
	} type;
	int bid;
	union {
		Ins *ins;
		Phi *phi;
	} u;
};

struct Tmp {
	char name[NString];
	Use *use;
	uint ndef, nuse;
	uint cost;
	short slot;
	short cls;
	struct {
		int r;
		bits m;
	} hint;
	int phi;
	int visit;
};

struct Con {
	enum {
		CUndef,
		CBits,
		CAddr,
	} type;
	char label[NString];
	union {
		int64_t i;
		double d;
		float s;
	} bits;
	char flt; /* 1 to print as s, 2 to print as d */
	char local;
};

typedef struct Addr Addr;

struct Addr { /* x64 addressing */
	Con offset;
	Ref base;
	Ref index;
	int scale;
};

struct Fn {
	Blk *start;
	Tmp *tmp;
	Con *con;
	Mem *mem;
	int ntmp;
	int ncon;
	int nmem;
	int nblk;
	int retty; /* index in typ[], -1 if no aggregate return */
	Ref retr;
	Blk **rpo;
	bits reg;
	int slot;
	char export;
	char name[NString];
};

struct Typ {
	char name[NString];
	int dark;
	ulong size;
	int align;

	struct {
		uint isflt:1;
		uint ispad:1;
		uint len:30;
	} seg[NSeg+1];
};

struct Dat {
	enum {
		DStart,
		DEnd,
		DName,
		DAlign,
		DB,
		DH,
		DW,
		DL,
		DZ
	} type;
	union {
		int64_t num;
		double fltd;
		float flts;
		char *str;
		struct {
			char *nam;
			int64_t off;
		} ref;
	} u;
	char isref;
	char isstr;
	char export;
};


/* main.c */
enum Asm {
	Gasmacho,
	Gaself,
};
extern char debug['Z'+1];
extern int stackprotection;
extern int diversify;

/* util.c */
extern Typ typ[NTyp];
extern Ins insb[NIns], *curi;
void die_(char *, char *, ...) __attribute__((noreturn));
void *emalloc(size_t);
void *alloc(size_t);
void freeall(void);
Blk *blknew(void);
void blkdel(Blk *);
void emit(int, int, Ref, Ref, Ref);
void emiti(Ins);
void idup(Ins **, Ins *, ulong);
Ins *icpy(Ins *, Ins *, ulong);
void *vnew(ulong, size_t);
void vgrow(void *, ulong);
int clsmerge(short *, short);
int phicls(int, Tmp *);
Ref newtmp(char *, int, Fn *);
void chuse(Ref, int, Fn *);
Ref getcon(int64_t, Fn *);
void addcon(Con *, Con *);
void dumpts(BSet *, Tmp *, FILE *);

void bsinit(BSet *, uint);
void bszero(BSet *);
uint bscount(BSet *);
void bsset(BSet *, uint);
void bsclr(BSet *, uint);
void bscopy(BSet *, BSet *);
void bsunion(BSet *, BSet *);
void bsinter(BSet *, BSet *);
void bsdiff(BSet *, BSet *);
int bsequal(BSet *, BSet *);
int bsiter(BSet *, int *);

static inline int bshas(BSet *bs, uint elt)
{
	assert(elt < bs->nt * NBit);
	return (bs->t[elt/NBit] & BIT(elt%NBit)) != 0;
}

/* parse.c */
extern OpDesc opdesc[NOp];
void parse(FILE *, char *, void (Dat *), void (Fn *));
void printfn(Fn *, FILE *);
void printref(Ref, Fn *, FILE *);
void err(char *, ...) __attribute__((noreturn));

/* mem.c */
void memopt(Fn *);

/* ssa.c */
void filluse(Fn *);
void fillpreds(Fn *);
void fillrpo(Fn *);
void ssa(Fn *);
void ssacheck(Fn *);

/* copy.c */
void copy(Fn *);

/* fold.c */
void fold(Fn *);

/* live.c */
void liveon(BSet *, Blk *, Blk *);
void filllive(Fn *);

/* abi: sysv.c */
extern int rsave[/* NRSave */];
extern int rclob[/* NRClob */];
bits retregs(Ref, int[2]);
bits argregs(Ref, int[2]);
void abi(Fn *);

/* isel.c */
void isel(Fn *);

/* spill.c */
void fillcost(Fn *);
void spill(Fn *);

/* rega.c */
void rega(Fn *);

/* emit.c */
extern char *locprefix;
extern char *symprefix;
void emitfn(Fn *, FILE *);
void emitdat(Dat *, FILE *);
int stashfp(int64_t, int);
void emitfin(FILE *);
void emitinit(FILE *);

/* random.c */
int openrnd();
int closernd();
int get_random_0to3();
