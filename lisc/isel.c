#include "lisc.h"

/* For x86_64, we have to:
 *
 * - check that constants are used only in
 *   places allowed by the machine
 *
 * - explicit machine register contraints
 *   on instructions like division.
 *
 * - lower calls (future, I have to think
 *   about their representation and the
 *   way I deal with structs/unions in the
 *   ABI)
 */

extern Ins insb[NIns], *curi; /* shared work buffer */

static void
emit(short op, Ref to, Ref arg0, Ref arg1)
{
	if (curi == insb)
		diag("isel: too many instructions");
	*--curi = (Ins){op, to, {arg0, arg1}};
}

static int
newtmp(int type, Fn *fn)
{
	static int n;
	int t;

	t = fn->ntmp++;
	fn->tmp = realloc(fn->tmp, fn->ntmp * sizeof fn->tmp[0]);
	if (!fn->tmp)
		diag("isel: out of memory");
	fn->tmp[t] = (Tmp){.type = type};
	sprintf(fn->tmp[t].name, "isel%d", ++n);
	return t;
}

static void
sel(Ins *i, Fn *fn)
{
	int t;
	Ref r0, r1;

	switch (i->op) {
	case ODiv:
		r0 = REG(RAX);
		r1 = REG(RDX);
	if (0) {
	case ORem:
		r0 = REG(RDX);
		r1 = REG(RAX);
	}
		emit(OCopy, i->to, r0, R);
		emit(OCopy, R, r1, R);
		if (rtype(i->arg[1]) == RCon) {
			/* immediates not allowed for
			 * divisions in x86
			 */
			t = newtmp(fn->tmp[i->to.val].type, fn);
			r0 = TMP(t);
		} else
			r0 = i->arg[1];
		emit(OXDiv, R, r0, R);
		emit(OSign, REG(RDX), REG(RAX), R);
		emit(OCopy, REG(RAX), i->arg[0], R);
		if (rtype(i->arg[1]) == RCon)
			emit(OCopy, r0, i->arg[1], R);
		break;
	case OAdd:
	case OSub:
	case OCopy:
		emit(i->op, i->to, i->arg[0], i->arg[1]);
		break;
	default:
		diag("isel: non-exhaustive implementation");
	}
}

/* instruction selection
 */
void
isel(Fn *fn)
{
	Blk *b;
	Ins *i;
	uint nins;

	for (b=fn->start; b; b=b->link) {
		curi = &insb[NIns];
		for (i=&b->ins[b->nins]; i!=b->ins;) {
			sel(--i, fn);
		}
		nins = &insb[NIns] - curi;
		free(b->ins);
		b->ins = alloc(nins * sizeof b->ins[0]);
		memcpy(b->ins, curi, nins * sizeof b->ins[0]);
		b->nins = nins;
	}
}
