#include "Definitions.hh"

namespace Beam {

std::map<SumStat, SumStatStruct> SumStatDict =
	{{alpha,	{"alpha", 
		  	"Transverse alpha function", 
		  	"#alpha_{#perp}", 
		  	"", 
		  	false}},
	 {beta,  	{"beta", 
		  	"Transverse beta function", 
		  	"#beta_{#perp}", 
		  	"mm", 
		  	false}},
	 {gamma,  	{"gamma", 
		  	"Transverse gamma function", 
		  	"#gamma_{#perp}", 
		  	"mm^{-1}", 
		  	false}},
	 {mecl, 	{"mecl", 
		  	"Mechanical angular momentum", 
		  	"#kappa#beta_{#perp} #minus#font[12]{L}", 
		  	"mm^{-1}", 
		  	false}},
	 {eps,   	{"eps", 
		  	"Transverse RMS geometric emittance", 
		  	"#epsilon_{#perp}", 
		  	"mm", 
		  	false}},
	 {neps,   	{"eps", 
		  	"Transverse RMS normalised emittance", 
		  	"#epsilon_{#perp}", 
		  	"mm", 
		  	false}},
	 {amp,   	{"amp", 
		  	"Amplitude quantile", 
		  	"A", 
		  	"mm", 
		  	true}},
	 {subeps,   	{"subeps", 
		  	"Subemittance", 
		  	"e", 
		  	"mm", 
		  	true}},
	 {vol,   	{"vol", 
		  	"Fractional emittance", 
		  	"#epsilon", 
		  	"mm^{2}(MeV/c)^{2}", 
		  	true}},
	 {den,   	{"den", 
		  	"Density", 
		  	"#rho", 
		  	"mm^{-2}(MeV/c)^{-2}", 
		  	true}},
	 {mom,   	{"mom", 
		  	"Average momentum", 
		  	"#bar{p}", 
		  	"MeV/c", 
		  	false}},
	 {trans,  	{"trans", 
		  	"Transmission", 
		  	"Transmission", 
		  	"%", 
		  	false}},
	 {disp,  	{"disp", 
		  	"Dispersion", 
		  	"D", 
		  	"mm", 
		  	false}}};
}
