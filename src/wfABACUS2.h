#ifndef WFABACUS2_H
#define WFABACUS2_H
#include "wavefunc.h"
#include "gfun.h"
#include "binfstream.h"
#include <fstream>
//read wave function in ABACUS
class WfABACUS2
{
	public:
	WfABACUS2(){
	wk=nullptr;
	energy=nullptr;
	occ=nullptr;
	};
	~WfABACUS2(){
		pclean(wk);
		this->clean();
	};
	void readWF(Wavefunc &,  int&);
	void readOUT(Wavefunc &);
	void readOCC(Wavefunc &, int&);
	void clean();
	private:
	binfstream rwswf;
	ifstream ifsocc;
	ifstream ifskwt;
	
	private:
	Vector3<double> axes[3];
	int nband;
	double *energy,*occ;
	int nkpoint;
	int ngtot;
	int num_ele;
	double * wk;
	double alat;
	int nx, ny, nz;

};
#endif
