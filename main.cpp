
#include "ConfigFile.h"

#include "nr3.h"
#include "ran.h"

#include "vecmanip.h"
#include "init_random.h"
#include "deriv.h"
#include "integrate.h"
#include "density.h"
#include "orientation.h"
#include "flux.h"
#include "walls.h"
#include "bfield.h"

#include <iostream>
#include <vector>
#include <vector>
#include <string>


using namespace std;

typedef vector<double> M1;
typedef vector<vector<double> > M2;
typedef vector<vector<vector<double> > > M3;


int main()
{

	int N;		// number of particles
	int navg;	// number of averages
	int seed;	// seed for the random number gen.
	int nbin;	// number of bins
	double m;	// particle mass
	double Dr; 	// rot. diff. const.
	double B;	// magnetic field strength
	double v0;	// self-propulsoin speed
	double w0;	// number of periods of the magnetic field
	double dt;	// time step for the integration
	double tsamp; // time between calculating averages
	double teq;	// equilibration time
	double L;	// box size
	int nprint; // print status every nprint

	string wallType; // type of wall: none, tube, disk, doughnut 
	double Ri;  	// inner radius of doughnut (only used if type == doughnut)
	double Ro;		// outer radius of disk or dough. (only used in those cases)
	double sig;		// sigma of LJ wall (only used if wallType != none)
	double eps;		// epsilon of LJ wall (%)
	double rco;		// cutoff of potential
	
	const string inputName = "input.txt";

	ConfigFile config(inputName);
	N = config.read<int>("N");
	navg = config.read<int>("navg");
	seed = config.read<int>("seed");
	nbin = config.read<int>("nbin");
	m = config.read<double>("m");
	Dr = config.read<double>("Dr");
	B = config.read<double>("B");
	v0 = config.read<double>("v0");
	w0 = config.read<double>("w0");
	dt = config.read<double>("dt");
	tsamp = config.read<double>("tsamp");
	teq = config.read<double>("teq");
	L = config.read<double>("L");
	nprint = config.read<int>("nprint");
	wallType = config.read<string>("wallType");
	Ri = config.read<double>("Ri");
	Ro = config.read<double>("Ro");
	sig = config.read<double>("sigma");
	eps = config.read<double>("epsilon");
	rco = config.read<double>("rco");
	double w = w0*2*acos(-1.)/L;
	double bs = L/(nbin);


	// define walls
	NoWall wallNone;
	TubeX wallTube(sig,eps,rco,L);
	Disk wallDisk(sig,eps,rco,Ro);
	Doughnut wallDough(sig,eps,rco,Ri,Ro);
	Wall* wall_ptr;
	// use wall according to wallType
	if(wallType == "none") {
		wall_ptr = &wallNone;
	}else if(wallType == "tube") {
		wall_ptr = &wallTube;
	}else if(wallType == "disk") {
		wall_ptr = &wallDisk;
	}else if(wallType == "doughnut") {
		wall_ptr = &wallDough;
	} else {
		cerr << "invalid wallType\n";
		return 1;
	}

	// orientation as a function of x,y,z
	M2 px(nbin,M1(3,0.));
	M2 py(nbin,M1(3,0.));
	M2 pz(nbin,M1(3,0.));
	// pxN &c. contains the number of particles in each bin
	M1 pxN(nbin,0);
	M1 pyN(nbin,0);
	M1 pzN(nbin,0);

	// density in the xy plane
	M2 rho(nbin,M1(nbin,0.));

	// flux in the xy plane
	// in the x direction
	M2 fx(nbin,M1(nbin,0.));
	// in the y direction
	M2 fy(nbin,M1(nbin,0.));


	// bins
	M1 bins(nbin,0.);
	for(int i=0;i<nbin;++i)
		bins[i] = (i+.5)*bs;


	// the random number generator
	Ranq2 ranNR(seed);

	// vectors describing the state of the system
	M2 r(N,M1(3));
	M2 dr(N,M1(3));
	M2 v(N,M1(3));
	M2 p(N,M1(3));



	// initialize
	init_r(r,L,ranNR);
	init_v(v,0.01,ranNR);
	init_p(p,ranNR);

	// the deriv objecs takes care of 
	// a single step of dt
	Deriv deriv(N,m,Dr,v0,B,w,wall_ptr,ranNR);

	// integrates uses the deriv object
	// to increment the system teq in time,
	// in steps of dt
	integrate(r,dr,v,p,deriv,teq,dt);
	double t = 0;
	for(int n=0;n<navg;++n) {
		if(n%nprint==0) cout << n << '\t' << navg << endl;
		integrate(r,dr,v,p,deriv,tsamp-dt,dt);
		// increment time by dt s.t. the last distplacement
		// is dt
		deriv(r,dr,v,p,dt);
		fluxXY(fx,fy,r,dr,v,bs,L,nbin);	
		// calculate orientation
		orientation(r,p,px,pxN,py,pyN,pz,pzN,bs,L);

		// calculate density
		density(r,rho,bs,L,nbin);

	}

	// normalize the flux and density
	for(int i=0;i<nbin;++i) {
		for(int j=0;j<nbin;++j){
			fx[i][j] /= navg*N*bs*bs;
			fy[i][j] /= navg*N*bs*bs;

			rho[i][j] /= navg*N*bs*bs;
		}
	}



	for(int i=0;i<nbin;++i) {
		for(int j=0;j<3;++j) {
			// devide orientation by number of 
			// measurements per bin
			if(pxN[i]>0) px[i][j] /= pxN[i];
			if(pyN[i]>0) py[i][j] /= pyN[i];
			if(pzN[i]>0) pz[i][j] /= pzN[i];

		}
	}

	write_vec("bins.dat",bins);
	write_mat("px.dat",px);
	write_mat("py.dat",py);
	write_mat("pz.dat",pz);
	write_mat("rho.dat",rho);
	write_mat("fx.dat",fx);
	write_mat("fy.dat",fy);
	
	return 0;

}








