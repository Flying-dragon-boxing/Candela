#include "wfABACUS2.h"
#include "input.h"
#include <fstream>
#include <sstream>
#include <string>
#include <limits>

//created by qianrui on 2020-2-5

string getidwf( int &);
void locate(binfstream& ,string&,const string,int);
string findstr(string ,const string &); 
string findstr(string); 
	
void WfABACUS2::clean()
{
	pclean(wk);
	pclean(occ);
	pclean(energy);
}


void WfABACUS2::readOUT(Wavefunc & wf) 
{
	//open outfile
	string outname=INPUT.wfdirectory+"/running_scf.log";
	ifskwt.open(outname.c_str());
	if(!ifskwt)
	{
		cout<<"Erro in opening OUT file!"<<endl;
		exit(0);
	}
	//cout<<outname<<" has been opened."<<endl;
	string useless;
	string txt;
	string checkstr;

	//get lattice parameter
	searchead(ifskwt,txt,"Number",1);
	ifskwt>>useless>>useless>>txt>>useless>>alat;//in bohr
	checkstr="(Bohr)";
	ifnecheckv(txt,checkstr);
	// cout<<"lattice parameter: "<<alat<<" a.u."<<endl;
	
	//get crystal axes
	searchead(ifskwt,txt,"Lattice",2);
	ifskwt>>axes[0].x>>axes[0].y>>axes[0].z;
	ifskwt>>axes[1].x>>axes[1].y>>axes[1].z;
	ifskwt>>axes[2].x>>axes[2].y>>axes[2].z;
	for(int i=0;i<3;i++)
	{
		axes[i]=alat*axes[i];
	}
	// cout<<"axes vector:"<<endl;
	// for(int i=0;i<3;i++)
    // {
	// 	cout<<axes[i].x<<'\t'<<axes[i].y<<'\t'<<axes[i].z<<endl;
	// }
	INPUT.celldm1=axes[0].norm();
	INPUT.celldm2=axes[1].norm();
	INPUT.celldm3=axes[2].norm();
	double vol;
	vol=axes[0].x*(axes[1].y*axes[2].z-axes[2].y*axes[1].z)+ axes[0].y*(axes[2].x*axes[1].z-axes[1].x*axes[2].z)+ axes[0].z*(axes[1].x*axes[2].y-axes[2].x*axes[1].y);//vol in bohr^3
    vol=abs(vol);
    INPUT.vol=vol;
	// cout << "volume: " << vol << " a.u.^3" << endl;
	
	//get GAMMA_ONLY
		INPUT.gamma=false;

	//get number of ele
	searchead(ifskwt,txt,"Autoset",1);
	stringstream ssr(txt);
	checkstr="electrons";
	ssr >> useless >> useless >> useless >> txt >> useless >> INPUT.nele;
	ifnecheckv(txt,checkstr);
	// cout<<"number of electrons: "<<INPUT.nele<<endl;

	//get nband
	searchead(ifskwt,txt,"Occupied",1);
	ifskwt>>useless>>useless>>useless>>useless>>txt>>useless>>nband;
	checkstr="(NBANDS)";
	ifnecheckv(checkstr,txt);
	wf.nband=nband;

	//get number of kpoint
	//TODO: to be convert to nkstot_ibz
	searchead(ifskwt,txt,"nkstot",2);
	nkpoint = str2int(txt.substr(7,10));
	// std::cout << "number of kpoints: " << nkpoint << std::endl;
	ifskwt >> txt;
	// ifnecheckv(txt,string("IBZ"));
	
	searchead(ifskwt,txt,"KPOINTS",1);
	INPUT.nkpoint=nkpoint;
	wk=new double [nkpoint];

	for(int i=0;i<nkpoint;i++)
	{
		ifskwt>>useless>>useless>>useless>>wk[i];
		// cout<<"wk"<<wk[i]<<endl;
		// wk[i] = 1;
	}

	searchead(ifskwt,txt,"FFT",1);
	// txt == grid for wave functions = [ 96, 96, 96 ]
	stringstream ss(txt);
	// std::cout << txt << std::endl;
	ss >> useless >> useless >> txt >> useless >> useless >> useless >> nx >> useless >> ny >> useless >> nz;
	checkstr="wave";
	ifnecheckv(checkstr,txt);
	// std::cout << "FFT grid: " << nx << " " << ny << " " << nz << std::endl;

	//get number of fermi energy
	searchead(ifskwt,txt,"E_exx",1);
	ifskwt>>txt>>useless>>INPUT.fermiE; //in eV
	checkstr="E_Fermi";
	// std::cout << "Fermi energy: " << INPUT.fermiE << " eV" << std::endl;
	ifnecheckv(checkstr,txt);
	wf.factor=1;
	ifskwt.close();
	
	//get occ
	energy=new double[nkpoint*nband];
	occ=new double [nkpoint*nband];
	string occname=INPUT.wfdirectory+"/eig.txt";
	ifsocc.open(occname.c_str());
	if(!ifsocc)
	{
		cout<<"Erro in opening OCC file!"<<endl;
		exit(0);
	}
	for(int ik = 0; ik < nkpoint; ++ik)
	{
		ifsocc>>useless;
		getline(ifsocc,txt);
		getline(ifsocc,txt);
		getline(ifsocc,txt);
		for(int i=0;i<nband;i++)
		{
			ifsocc>>useless>>energy[ik*nband+i]>>occ[ik*nband+i];
			// std::cout<<energy[ik*nband+i]<<' '<<occ[ik*nband+i]<<std::endl;
		}
	}
	ifsocc.close();

}

void WfABACUS2::readOCC(Wavefunc & wf, int & ik) 
{
	wf.occ=new double [nband];
	wf.eigE=new double [nband];
	wf.wk=wk[ik];
	for(int i=0;i<nband;i++)
	{
		wf.eigE[i]=energy[ik*nband+i];
		wf.occ[i]=occ[ik*nband+i]/2;
	}
}


void WfABACUS2::readWF(Wavefunc &wf, int &ik)
{
	//open wavefunc file and initialize
	string wfname=INPUT.wfdirectory+"/wfs1k"+int2str(ik+1)+"_pw.dat";
	rwswf.open(wfname,"r");
	if(!rwswf)
	{
		cout<<"Error in opening WAVEFUNC*.dat file!"<<endl;
		exit(0);
	}

	int strw,endrw;
	int ik_2,nkpoint_2,nband_2;
	double lat0,invlat0,wk_2,ecut;
	double kx_cry,ky_cry,kz_cry;
	
	//get data
	rwswf>>strw>>ik_2>>nkpoint_2>>kx_cry>>ky_cry>>kz_cry>>wk_2>>ngtot>>nband_2>>ecut>>lat0>>invlat0>>endrw;
	ik_2-=1;
	ifnecheckv(ik,ik_2);
	// std::cout << nkpoint_2 << " " << nkpoint << std::endl;
	ifnecheckv(nkpoint_2,nkpoint);
	ifnecheckv(nband,nband_2);
	ifnecheckv(strw,endrw);
	// cout<<"ngtot: "<<ngtot<<endl;
	wf.ngtot=ngtot;
	//get kpoint vector	
	wf.kpoint_x=kx_cry * invlat0;
	wf.kpoint_y=ky_cry * invlat0;
	wf.kpoint_z=kz_cry * invlat0;
	// cout<<"kpoint_vector: ("<<wf.kpoint_x<<','<<wf.kpoint_y<<','<<wf.kpoint_z<<")\n";

	//get inverse lattice matrix
	double b[9];
	rwswf>>strw;
	rwread(rwswf, b, 9);
	rwswf>>endrw;
	ifnecheckv(strw,endrw);
	
	//get gkk
	rwswf>>strw;
	wf.gkk_x=new double [ngtot];
	wf.gkk_y=new double [ngtot];
	wf.gkk_z=new double [ngtot];

	int inttmp=((ngtot)*3)*4;
	ifnecheckv(strw,inttmp);

	wf.ig0 = -1;
	int max_ix = std::numeric_limits<int>::min();
	int max_iy = std::numeric_limits<int>::min();
	int max_iz = std::numeric_limits<int>::min();

	for(int i=0;i<ngtot;i++)
	{

		int ix,iy,iz;
		rwswf>>ix>>iy>>iz;

		// Track the greatest ix, iy, iz
		if(ix > max_ix) max_ix = ix;
		if(iy > max_iy) max_iy = iy;
		if(iz > max_iz) max_iz = iz;

		if (ix >= int(nx / 2) + 1)
		{
			ix -= nx;
		}
		if (iy >= int(ny / 2) + 1)
		{
			iy -= ny;
		}
		if (iz >= int(nz / 2) + 1)
		{
			iz -= nz;
		}
		
		double gx, gy, gz;
		gx = b[0] * ix + b[3] * iy + b[6] * iz;
		gy = b[1] * ix + b[4] * iy + b[7] * iz;
		gz = b[2] * ix + b[5] * iy + b[8] * iz;

		wf.gkk_x[i] = gx * invlat0;
		wf.gkk_y[i] = gy * invlat0;
		wf.gkk_z[i] = gz * invlat0;

		if(pow(gx,2)+pow(gy,2)+pow(gz,2) < 1e-8)
		{
			wf.ig0 = i;
			// cout<<"ig0: "<<wf.ig0<<endl;
		}
	}
	// cout << "Max ix: " << max_ix << ", Max iy: " << max_iy << ", Max iz: " << max_iz << endl;
	rwswf>>endrw;
	ifnecheckv(strw,endrw);

	//read wavefunc
	wf.Wavegg=new complex<double>[nband*ngtot];
	//double *sum;
	//sum=new double [nband];
	
	//read WF	
	for(int i=0;i<nband;i++)
	{
		//sum[i]=0;
		rwswf>>strw;
		for(int j=0;j<ngtot;j++)
		{
			int index=i*ngtot+j;
			rwswf>>wf.Wavegg[index];
			//sum[i]+=pow(wf.Wavegg[index].real(),2)+pow(wf.Wavegg[index].imag(),2);
		}
		rwswf>>endrw;
		ifnecheckv(strw,endrw);
	}
	// for(int i=0;i<nband;i++)
	// {
	// 	cout<<"iband: "<<i<<"\tsum : "<<sum[i]<<endl;
	// 	cout<<"iband "<<i<<" read"<<endl;
	// 	for(int j=0;j<ngtot;j++)
	// 		cout<<wf.Wavegg[i*ngtot+j]<<' ';
	// 	cout<<endl;
	// }
	return;
}



 


