#include <cstddef>
#include <cstring>
#include <set>
#include <map>
#include <sstream>
#include <cmath>

#include "predicates.h"

#include "mesh.h"


namespace fmesh {

  


  /*
    T-E+V=2
    
    closed 2-manifold triangulation:
    E = T*3/2
    T = 2*V-4

    simply connected 2-manifold triangulation:
    T <= 2*V-5

  */

  Mesh::Mesh(Mtype manifold_type,
	     size_t V_capacity,
	     bool use_VT,
	     bool use_TTi) : type_(manifold_type),
			     Vcap_(V_capacity), Tcap_(2*V_capacity),
			     nV_(0), nT_(0),
			     use_VT_(use_VT), use_TTi_(use_TTi)
  {
    if (Vcap_ > 0) {
      TV_ = new int[Vcap_][3];
      TT_ = new int[Tcap_][3];
      if (use_VT_)
	VT_ = new int[Vcap_];
      else
	VT_ = NULL;
      if (use_TTi_)
	TTi_ = new int[Tcap_][3];
      else
	TTi_ = NULL;
      S_ = new double[Vcap_][3];
    } else {
      TV_ = NULL;
      TT_ = NULL;
      VT_ = NULL;
      TTi_ = NULL;
      S_ = NULL;
    }
    X11_ = NULL;
  };

  Mesh::~Mesh()
  {
    clear();
  }

  Mesh& Mesh::clear()
  {
    Vcap_ = 0;
    Tcap_ = 0;
    nV_ = 0;
    nT_ = 0;
    use_VT_ = false;
    use_TTi_ = false;
    if (TV_) { delete[] TV_; TV_ = NULL; }
    if (TT_) { delete[] TT_; TT_ = NULL; }
    if (VT_) { delete[] VT_; VT_ = NULL; }
    if (TTi_) { delete[] TTi_; TTi_ = NULL; }
    if (S_) { delete[] S_; S_ = NULL; }
    if (X11_) { delete X11_; X11_ = NULL; }
    return *this;
  }

  Mesh& Mesh::operator=(const Mesh& M)
  {
    clear();
    type_ = M.type_;
    useVT(M.use_VT_);
    useTTi(M.use_TTi_);
    if (M.X11_) {
      X11_ = new Xtmpl(*M.X11_);
    } else {
      X11_ = NULL;
    }
    S_set(M.S_,M.nV_);
    TV_set(M.TV_,M.nT_);
    return *this;
  }

  Mesh& Mesh::check_capacity(size_t nVc, size_t nTc)
  {
    if ((nVc <= Vcap_) && (nTc <= Tcap_))
      return *this;
    while ((nVc > Vcap_) || (nTc > Tcap_)) {
      Vcap_ = Vcap_+Mesh_V_capacity_step_size;
      Tcap_ = 2*Vcap_;
    }

    int (*TV)[3] = new int[Vcap_][3];
    int (*TT)[3] = new int[Tcap_][3];
    int (*VT) = NULL;
    if (use_VT_) VT = new int[Vcap_];
    int (*TTi)[3] = NULL;
    if (use_TTi_) TTi = new int[Tcap_][3];
    double (*S)[3] = new double[Vcap_][3];

    if (TV_) {
      if (TV_) memcpy(TV,TV_,sizeof(int)*nV_*3);
      if (TT_) memcpy(TT,TT_,sizeof(int)*nT_*3);
      if (VT_) memcpy(VT,VT_,sizeof(int)*nV_);
      if (TTi_) memcpy(TTi,TTi_,sizeof(int)*nT_*3);
      if (S_) memcpy(S,S_,sizeof(double)*nV_*3);
      if (TV_) delete[] TV_;
      if (TT_) delete[] TT_;
      if (VT_) delete[] VT_;
      if (TTi_) delete[] TTi_;
      if (S_) delete[] S_;
    }

    TV_ = TV;
    TT_ = TT;
    VT_ = VT;
    TTi_ = TTi;
    S_ = S;
    return *this;
  };



  Mesh& Mesh::rebuildTT()
  {
    typedef std::pair<int,int> E_Type;
    typedef std::map<E_Type,int> ET_Type;
    int t, vi;
    int* TVt;
    E_Type E0,E1;
    ET_Type::const_iterator Ei;
    ET_Type ET;
    /* Pass 1: */
    for (t=0; t<(int)nT_; t++) {
      TVt = TV_[t];
      for (vi=0; vi<3; vi++) {
	E0 = std::pair<int,int>(TVt[(vi+1)%3],TVt[(vi+2)%3]);
	E1 = std::pair<int,int>(TVt[(vi+2)%3],TVt[(vi+1)%3]);
	Ei = ET.find(E1);
	if (Ei != ET.end()) { /* Found neighbour */
	  TT_[t][vi] = Ei->second;
	} else { /* Either on boundary, or not added yet. */
	  TT_[t][vi] = -1;
	}
	ET.insert(ET_Type::value_type(E0,t));
      }
    }
    /*
    std::cout << TTO() << std::endl;
    for (Ei=ET.begin();Ei!=ET.end();Ei++) {
      std::cout << Ei->first.first << ' '
		<< Ei->first.second << ' '
		<< Ei->second << std::endl;
    }
    */

    /* Pass 2: */
    for (t=0; t<(int)nT_; t++) {
      TVt = TV_[t];
      for (vi=0; vi<3; vi++) {
	if (TT_[t][vi]>=0) continue;
	E1 = std::pair<int,int>(TVt[(vi+2)%3],TVt[(vi+1)%3]);
	Ei = ET.find(E1);
	if (Ei != ET.end()) { /* Found neighbour */
	  TT_[t][vi] = Ei->second;
	}
      }
    }

    return *this;
  }



  Mesh& Mesh::updateVT(const int v, const int t)
  {
    if ((!use_VT_) || (v>=(int)nV_) || (t>=(int)nT_) || (VT_[v]<0))
      return *this;
    VT_[v] = t;
    return *this;
  }

  Mesh& Mesh::setVT(const int v, const int t)
  {
    if ((!use_VT_) || (v>=(int)nV_) || (t>=(int)nT_))
      return *this;
    VT_[v] = t;
    return *this;
  }

  Mesh& Mesh::updateVTtri(const int t)
  {
    int vi;
    if ((!use_VT_) || (t>=(int)nT_) || (t<0))
      return *this;
    for (vi=0; vi<3; vi++)
      updateVT(TV_[t][vi],t);
    return *this;
  }

  Mesh& Mesh::setVTtri(const int t)
  {
    int vi;
    if ((!use_VT_) || (t>=(int)nT_) || (t<0))
      return *this;
    for (vi=0; vi<3; vi++)
      setVT(TV_[t][vi],t);
    return *this;
  }

  Mesh& Mesh::updateVTtri_private(const int t0)
  {
    if (!use_VT_) return *this;
    int t, vi;
    for (t=t0; t<(int)nT_; t++)
      for (vi=0; vi<3; vi++)
	updateVT(TV_[t][vi],t);
    return *this;
  }

  Mesh& Mesh::setVTv_private(const int v0)
  {
    if (!use_VT_) return *this;
    int v;
    for (v=v0; v<(int)nV_; v++)
      setVT(v,-1);
    return *this;
  }

  Mesh& Mesh::rebuildVT()
  {
    if (!use_VT_) {
      if (VT_) {
	delete[] VT_;
	VT_ = NULL;
      }
      return *this;
    }
    if (!Vcap_)
      return *this;
    if (!VT_)
      VT_ = new int[Vcap_];
    setVTv_private(0);
    updateVTtri_private(0);
    return *this;
  }

  Mesh& Mesh::rebuildTTi()
  {
    int t, vi, v, t2, vi2;
    if (!use_TTi_) {
      if (TTi_) {
	delete[] TTi_;
	TTi_ = NULL;
      }
      return *this;
    }
    if (!Tcap_)
      return *this;
    if (!TTi_)
      TTi_ = new int[Tcap_][3];
    for (t=0; t<(int)nT_; t++) {
      for (vi=0; vi<3; vi++) {
	v = TV_[t][vi];
	t2 = TT_[t][(vi+2)%3];
	if (t2>=0) {
	  for (vi2 = 0; (vi2<3) && (TV_[t2][vi2] != v); vi2++) { }
	  if (vi2<3) {
	    TTi_[t][(vi+2)%3] = (vi2+1)%3;
	  } else {
	    /* Error! This should never happen! */
	    std::cout << "ERROR\n";
	  }
	} else {
	  TTi_[t][(vi+2)%3] = -1;
	}
      }
    }
    return *this;
  }


  Mesh& Mesh::useVT(bool use_VT)
  {
    if (use_VT_ != use_VT) {
      if ((!use_VT_) && (VT_)) {
	/* This shouldn't happen. */
	delete[] VT_;
	VT_ = NULL;
      }
      use_VT_ = use_VT;
      rebuildVT();
    }
    return *this;
  }

  Mesh& Mesh::useTTi(bool use_TTi)
  {
    if (use_TTi_ != use_TTi) {
      if ((!use_TTi_) && (TTi_)) {
	/* This shouldn't happen. */
	delete[] TTi_;
	TTi_ = NULL;
      }
      use_TTi_ = use_TTi;
      rebuildTTi();
    }
    return *this;
  }

  Mesh& Mesh::useX11(bool use_X11,
		     int sx, int sy,
		     double minx,
		     double maxx,
		     double miny,
		     double maxy,
		     std::string name)
  {
    if (use_X11) {
      if (!X11_) { /* Init. */
	X11_ = new Xtmpl(sx,sy,minx,maxx,miny,maxy,name);
	redrawX11("");
      } else {
	X11_->reopen(sx,sy);
	X11_->setAxis(minx,maxx,miny,maxy);
	redrawX11("");
      }
    } else { /* Destroy. */
      if (X11_) { /* Destroy. */
	delete X11_;
	X11_ = NULL;
      }
    }
    return *this;
  }



  Mesh& Mesh::S_set(const double (*S)[3], int nV)
  {
    nV_ = 0; /* Avoid possible unnecessary copy. */
    S_append(S,nV);
    return *this;
  }
  
  Mesh& Mesh::TV_set(const int (*TV)[3], int nT)
  {
    nT_ = 0; /* Avoid possible unnecessary copy. */
    TV_append(TV,nT);
    return *this;
  }

  Mesh& Mesh::S_append(const double (*S)[3], int nV)
  {
    check_capacity(nV_+nV,0);
    memcpy(S_+nV_,S,sizeof(double)*nV*3);
    nV_ += nV;
    if (use_VT_)
      setVTv_private(nV_-nV);
    return *this;
  }

  void Mesh::redrawX11(std::string str)
  {
    if (!X11_) return;

    int v;
    double s[3][3];
    double s0[3];

    X11_->clear();
    for (int t=0;t<(int)nT_;t++) {
      s0[0] = 0.0;
      s0[1] = 0.0;
      s0[2] = 0.0;
      for (int vi=0;vi<3;vi++) {
	v = TV_[t][vi];
	for (int dim=0;dim<3;dim++) {
	  s[vi][dim] = S_[v][dim];
	  s0[dim] += s[vi][dim]/3;
	}
      }
      if (type_==Mtype_sphere) {
	{
	  double l = 0;
	  for (int dim=0;dim<3;dim++)
	    l += s0[dim]*s0[dim];
	  l = std::sqrt(l);
	  for (int dim=0;dim<3;dim++)
	    s0[dim] = s0[dim]/l;
	}
	double r0[3];
	double r1[3];
	double n[3];
	for (int dim=0;dim<3;dim++) {
	  r0[dim] = s[1][dim]-s[0][dim];
	  r1[dim] = s[2][dim]-s[0][dim];
	}
	n[0] = r0[1]*r1[2]-r0[2]*r1[1];
	n[1] = r0[2]*r1[0]-r0[0]*r1[2];
	n[2] = r0[0]*r1[1]-r0[1]*r1[0];
	if (n[2]<0) continue;
      }
      /* Draw triangle slightly closer to center. */
      if (type_==Mtype_sphere) {
	for (int vi=0;vi<3;vi++) {
	  for (int dim=0;dim<3;dim++)
	    s[vi][dim] = (s[vi][dim]-s0[dim])*0.975+s0[dim];
	  double l = 0;
	  for (int dim=0;dim<3;dim++)
	    l += s[vi][dim]*s[vi][dim];
	  l = std::sqrt(l);
	  for (int dim=0;dim<3;dim++)
	    s[vi][dim] = s[vi][dim]/l;
	}
	X11_->arc(s[0],s[1]);
	X11_->arc(s[1],s[2]);
	X11_->arc(s[2],s[0]);
      } else {
	for (int vi=0;vi<3;vi++)
	  for (int dim=0;dim<3;dim++)
	    s[vi][dim] = (s[vi][dim]-s0[dim])*0.975+s0[dim];
	X11_->line(s[0],s[1]);
	X11_->line(s[1],s[2]);
	X11_->line(s[2],s[0]);
      }
      /* Draw vertex indices even closer to center. */
      for (int vi=0;vi<3;vi++)
	for (int dim=0;dim<3;dim++)
	  s[vi][dim] = (s[vi][dim]-s0[dim])*0.8+s0[dim];
      for (int vi=0;vi<3;vi++) {
	std::ostringstream ss;
	ss << "(" << TV_[t][vi] << "," << TT_[t][vi] << ")";
	X11_->text(s[vi],ss.str());
      }
      /* Draw triangle indices at center. */
      {
	std::ostringstream ss;
	ss << "(" << t << ")";
	X11_->text(s0,ss.str());
      }
    }

    {
      std::string str0 = str;
      str0 += std::string(", continue");
      char* str_ = new char[str0.length()+1];
      str0.copy(str_,str0.length(),0);
      str_[str0.length()] = '\0';
      xtmpl_press_ret(str_);
      delete[] str_;
    }
  }
  
  Mesh& Mesh::TV_append(const int (*TV)[3], int nT)
  {
    check_capacity(0,nT_+nT);
    memcpy(TV_+nT_,TV,sizeof(int)*nT*3);
    nT_ += nT;
    if (use_VT_)
      updateVTtri_private(nT_-nT);
    rebuildTT();
    rebuildTTi();
    redrawX11(std::string("TV appended"));
    return *this;
  }




  double Mesh::edgeLength(const Dart& d) const
  {
    int t(d.t());
    if ((t<0) || (t>=(int)nT_)) return 0.0;

    double len;
    double s0[3];
    double s1[3];

    int v0 = TV_[d.t()][d.vi()];
    Dart dhelper(d);
    dhelper.alpha0();
    int v1 = TV_[dhelper.t()][dhelper.vi()];
    for (int dim=0;dim<3;dim++) {
      s0[dim] = S_[v0][dim];
      s1[dim] = S_[v1][dim];
    }

    switch (type_) {
    case Mesh::Mtype_manifold:
      /* TODO: Implement. */
      NOT_IMPLEMENTED;
      len = 1.0;
      break;
    case Mesh::Mtype_plane:
      double e[3];
      e[0] = s1[0]-s0[0];
      e[1] = s1[1]-s0[1];
      e[2] = s1[2]-s0[2];
      len = std::sqrt(e[0]*e[0]+e[1]*e[1]+e[2]*e[2]);
      break;
    case Mesh::Mtype_sphere:
      len = s0[0]*s1[0]+s0[1]*s1[1]+s0[2]*s1[2];
      len = std::acos(std::min(1.0,std::max(-1.0,len)));
      break;
    }

    return len;
  }

  double Mesh::triangleArea(int t) const
  {
    if ((t<0) || (t>=(int)nT_)) return 0.0;

    /* TODO: Implement. */
    NOT_IMPLEMENTED;

    return 1.0;
  }

  double Mesh::triangleCircumcircleRadius(int t) const
  {
    if ((t<0) || (t>=(int)nT_)) return 0.0;

    /* TODO: Implement. */
    NOT_IMPLEMENTED;

    return 1.0;
  }

  double Mesh::triangleShortestEdge(int t) const
  {
    if ((t<0) || (t>=(int)nT_)) return 0.0;

    double len;
    Dart d(*this,t);
    double len_min = edgeLength(d);
    d.orbit2();
    len = edgeLength(d);
    if (len < len_min)
      len_min = len;
    d.orbit2();
    len = edgeLength(d);
    if (len < len_min)
      len_min = len;

    return len_min;
  }

  double Mesh::triangleLongestEdge(int t) const
  {
    if ((t<0) || (t>=(int)nT_)) return 0.0;

    double len;
    Dart d(*this,t);
    double len_max = edgeLength(d);
    d.orbit2();
    len = edgeLength(d);
    if (len > len_max)
      len_max = len;
    d.orbit2();
    len = edgeLength(d);
    if (len > len_max)
      len_max = len;

    return len_max;
  }


  double Mesh::edgeEncroached(const Dart& d, const double s[3]) const
  /* > --> encroached */
  {
    int t(d.t());
    if ((t<0) || (t>=(int)nT_)) return -1.0;

    Dart dhelper(d);
    int v0 = TV_[t][dhelper.vi()];
    dhelper.orbit2();
    int v1 = TV_[t][dhelper.vi()];
    /* Construct a mirror of s reflected in v0-->v1 */
    double sm[3]; 
    switch (type_) {
    case Mesh::Mtype_manifold:
      //	return predicates::orient3d(M_->S[]);
    /* TODO: Implement. */
      NOT_IMPLEMENTED;
      sm[0] = 0.0;
      sm[1] = 0.0;
      sm[2] = 0.0;
      break;
    case Mesh::Mtype_plane:
      /* e = s1-s0
         sv = s-s0
         sm = s0 + (2*e*e'*sv/elen2 - sv)
            = s0 + (2*e*se - sv)
       */
      {
	double s0[3];
	s0[0] = S_[v0][0];
	s0[1] = S_[v0][1];
	s0[2] = S_[v0][2];
	double e[3];
	e[0] = S_[v1][0]-s0[0];
	e[1] = S_[v1][1]-s0[1];
	e[2] = S_[v1][2]-s0[2];
	double elen2 = e[0]*e[0]+e[1]*e[1]+e[2]*e[2];
	double sv[3];
	sv[0] = s[0]-s0[0];
	sv[1] = s[1]-s0[1];
	sv[2] = s[2]-s0[2];
	double se = (sv[0]*e[0]+sv[1]*e[1]+sv[2]*e[2])/elen2;
	sm[0] = s0[0] + (2*se*e[0] - sv[0]);
	sm[1] = s0[1] + (2*se*e[1] - sv[1]);
	sm[2] = s0[2] + (2*se*e[2] - sv[2]);
      }
      break;
    case Mesh::Mtype_sphere:
      /* TODO: Implement. */
      NOT_IMPLEMENTED;
      /*      Point zero = {0.,0.,0.}; */
      sm[0] = 0.0;
      sm[1] = 0.0;
      sm[2] = 0.0;
      break;
    }
    
    switch (type_) {
    case Mesh::Mtype_manifold:
      //	return predicates::orient3d(M_->S[]);
      /* TODO: Implement. */
      NOT_IMPLEMENTED;
      break;
    case Mesh::Mtype_plane:
      return predicates::incircle(S_[v0],S_[v1],s,sm);
      break;
    case Mesh::Mtype_sphere:
      //      return -predicates::orient3d(S_[v0],S_[v1],S_[v2],s);
      /* TODO: Implement. */
      NOT_IMPLEMENTED;
      break;
    }
    /* This should never be reached. */
    return 0.0;
  }

  double Mesh::encroachedQuality(const Dart& d) const
  /* > --> encroached */
  {
    int t(d.t());
    if ((t<0) || (t>=(int)nT_)) return -1.0;

    Dart dhelper(d);
    dhelper.orbit2rev();
    
    double encr = edgeEncroached(d,S_[TV_[t][dhelper.vi()]]);

    dhelper.orbit2rev();
    std::cout << "encroachedQ("
	      << TV_[t][d.vi()] << "," << TV_[t][dhelper.vi()]
	      << ") = " << encr << std::endl;

    return encr;
  }

  double Mesh::skinnyQuality(int t) const
  {
    if ((t<0) || (t>=(int)nT_)) return 0.0;

    double skinny = (triangleCircumcircleRadius(t) / 
		     triangleShortestEdge(t));

    std::cout << "skinnyQ(" << t << ") = " << skinny << std::endl;

    return skinny;
  }

  double Mesh::bigQuality(int t) const
  {
    return triangleLongestEdge(t);
  }




  double Mesh::inLeftHalfspace(const Dart& d, const double s[3]) const
  {
    Dart dhelper = d;
    int v0, v1;
    if (d.isnull()) return 0.0; /* TODO: should show a warning somewhere... */
    const int* tp = TV_[dhelper.t()];
    v0 = tp[dhelper.vi()];
    dhelper.orbit2();
    v1 = tp[dhelper.vi()];
    switch (type_) {
    case Mesh::Mtype_manifold:
      //	return predicates::orient3d(M_->S[]);
      NOT_IMPLEMENTED;
      break;
    case Mesh::Mtype_plane:
      return predicates::orient2d(S_[v0],S_[v1],s);
      break;
    case Mesh::Mtype_sphere:
      Point zero = {0.,0.,0.};
      return -predicates::orient3d(S_[v0],S_[v1],zero,s);
      break;
    }
    /* This should never be reached. */
    return 0.0;
  }

  double Mesh::inCircumcircle(const Dart& d, const double s[3]) const
  {
    Dart dhelper = d;
    int v0, v1, v2;
    if (d.isnull()) return 0.0; /* TODO: should show a warning somewhere... */
    const int* tp = TV_[dhelper.t()];
    v0 = tp[dhelper.vi()];
    dhelper.orbit2();
    v1 = tp[dhelper.vi()];
    dhelper.orbit2();
    v2 = tp[dhelper.vi()];
    switch (type_) {
    case Mesh::Mtype_manifold:
      //	return predicates::orient3d(M_->S[]);
      break;
    case Mesh::Mtype_plane:
      return predicates::incircle(S_[v0],S_[v1],S_[v2],s);
      break;
    case Mesh::Mtype_sphere:
      return -predicates::orient3d(S_[v0],S_[v1],S_[v2],s);
      break;
    }
    /* This should never be reached. */
    return 0.0;
  }

  bool Mesh::circumcircleOK(const Dart& d) const
  {
    Dart dhelper = d;
    int v;
    double result;
    if (d.isnull()) return true; /* TODO: should show a warning somewhere... */
    if (d.onBoundary()) return true; /* Locally optimal, OK. */
    dhelper.orbit0rev().orbit2();
    v = TV_[dhelper.t()][dhelper.vi()];
    result = inCircumcircle(d,S_[v]);
    std::cout << "Dart=" << d
	      << " Node=" << v
	      << std::scientific << " result=" << result
	      << std::endl;
    if  (result > MESH_EPSILON)
      return false;
    /* For robusness, check with the reverse dart as well: */
    dhelper = d;
    dhelper.orbit2rev();
    v = TV_[dhelper.t()][dhelper.vi()];
    dhelper.orbit2();
    dhelper.orbit1();
    result = inCircumcircle(dhelper,S_[v]);
    std::cout << "Dart=" << dhelper
	      << " Node=" << v
	      << std::scientific << " result=" << result
	      << std::endl;
    if  (result > MESH_EPSILON)
      return false;
    return true;
  }


  /*! \brief Swap an edge

     \verbatim
       2         2
      /0\       /|\
     0d--1 --> 00|11
      \1/       \d/
       3         3
     \endverbatim
     Dart 0-1 --> 3-2
    
  */
  Dart Mesh::swapEdge(const Dart& d)
  {
    Dart dhelper = d;
    int vi;
    int v_list[4];
    int t0, t1;
    int tt_list[4];
    int tti_list[4];
    if (d.edir()<0) dhelper.alpha1(); /* Correct dart orientation */

    /* Step 1: Store geometry information. */
    t0 = dhelper.t();
    vi = dhelper.vi();
    v_list[0] = TV_[t0][vi];
    tt_list[0] = TT_[t0][vi];
    if (use_TTi_) tti_list[0] = TTi_[t0][vi];
    dhelper.orbit2();
    vi = dhelper.vi();
    v_list[1] = TV_[t0][vi];
    tt_list[1] = TT_[t0][vi];
    if (use_TTi_) tti_list[1] = TTi_[t0][vi];
    dhelper.orbit2();
    v_list[2] = TV_[t0][dhelper.vi()];
    dhelper.orbit2rev().orbit0();
    t1 = dhelper.t();
    if (t0 == t1) { dhelper = d; return dhelper; } /* ERROR: Boundary edge */
    vi = dhelper.vi();
    tt_list[2] = TT_[t1][vi];
    if (use_TTi_) tti_list[2] = TTi_[t1][vi];
    dhelper.orbit2();
    vi = dhelper.vi();
    tt_list[3] = TT_[t1][vi];
    if (use_TTi_) tti_list[3] = TTi_[t1][vi];
    dhelper.orbit2();
    v_list[3] = TV_[t1][dhelper.vi()];

    /* Step 2: Overwrite with new triangles. */
    TV_[t0][0] = v_list[0];
    TV_[t0][1] = v_list[3];
    TV_[t0][2] = v_list[2];
    TT_[t0][0] = t1;
    TT_[t0][1] = tt_list[1];
    TT_[t0][2] = tt_list[2];
    if (use_TTi_) {
      TTi_[t0][0] = 0;
      TTi_[t0][1] = tti_list[1];
      TTi_[t0][2] = tti_list[2];
    }
    TV_[t1][0] = v_list[1];
    TV_[t1][1] = v_list[2];
    TV_[t1][2] = v_list[3];
    TT_[t1][0] = t0;
    TT_[t1][1] = tt_list[3];
    TT_[t1][2] = tt_list[0];
    if (use_TTi_) {
      TTi_[t1][0] = 0;
      TTi_[t1][1] = tti_list[3];
      TTi_[t1][2] = tti_list[0];
    }

    /* Step 3: Relink neighbouring triangles. */
    if (use_TTi_) {
      if (TT_[t0][1]>=0) TTi_[TT_[t0][1]][TTi_[t0][1]] = 1;
      if (TT_[t0][2]>=0) TTi_[TT_[t0][2]][TTi_[t0][2]] = 2;
      if (TT_[t1][1]>=0) TTi_[TT_[t1][1]][TTi_[t1][1]] = 1;
      if (TT_[t1][2]>=0) TTi_[TT_[t1][2]][TTi_[t1][2]] = 2;
      if (TT_[t0][1]>=0) TT_[TT_[t0][1]][TTi_[t0][1]] = t0;
      if (TT_[t0][2]>=0) TT_[TT_[t0][2]][TTi_[t0][2]] = t0;
      if (TT_[t1][1]>=0) TT_[TT_[t1][1]][TTi_[t1][1]] = t1;
      if (TT_[t1][2]>=0) TT_[TT_[t1][2]][TTi_[t1][2]] = t1;
    } else {
      if (TT_[t0][1]>=0) {
	dhelper = Dart(*this,t0,1,2).orbit0rev();
	dhelper.orbit2();
	TT_[dhelper.t()][dhelper.vi()] = t0;
      }
      if (TT_[t0][2]>=0) {
	dhelper = Dart(*this,t0,1,0).orbit0rev();
	dhelper.orbit2();
	TT_[dhelper.t()][dhelper.vi()] = t0;
      }
      if (TT_[t1][1]>=0) {
	dhelper = Dart(*this,t1,1,2).orbit0rev();
	dhelper.orbit2();
	TT_[dhelper.t()][dhelper.vi()] = t1;
      }
      if (TT_[t1][2]>=0) {
	dhelper = Dart(*this,t1,1,0).orbit0rev();
	dhelper.orbit2();
	TT_[dhelper.t()][dhelper.vi()] = t1;
      }
    }

    /* Link vertices to triangles */
    if (use_VT_) {
      setVTtri(t1);
      setVTtri(t0);
    }

    /* Debug code: */
    /* 
    std::cout << "TT is \n" << TTO();
    rebuildTT();
    std::cout << "TT should be \n" << TTO();
    if (use_TTi_) {
      std::cout << "TTi is \n" << TTiO();
      rebuildTTi();
      std::cout << "TTi should be \n" << TTiO();
    }
    */

    redrawX11("Edge swapped");
    
    return Dart(*this,t0,1,1);
  }
  
  /*!
     \verbatim
     2           2
    /|\         /|\
   / | \       /1d2\
  1 0|1 3 --> 1--v--3
   \ | /       \0|3/
    \d/         \|/
     0           0
     \endverbatim
   
     Dart 0-2 --> v-2
  */
  Dart Mesh::splitEdge(const Dart& d, int v)
  {
    Dart dhelper = d;
    int t, vi;
    int v0, v1, v2, v3;
    int t0, t1, t2, t3;
    int tt_list[4];
    int tti_list[4];
    if (d.edir()<0) dhelper.alpha0(); /* Correct dart orientation */

    /* Step 1: Store geometry information. */
    /* Go through t0: */
    t0 = dhelper.t();
    vi = dhelper.vi();
    v0 = TV_[t0][vi];
    tt_list[1] = TT_[t0][vi];
    if (use_TTi_) tti_list[1] = TTi_[t0][vi];
    dhelper.orbit2();
    vi = dhelper.vi();
    v2 = TV_[t0][vi];
    tt_list[0] = TT_[t0][vi];
    if (use_TTi_) tti_list[0] = TTi_[t0][vi];
    dhelper.orbit2();
    vi = dhelper.vi();
    v1 = TV_[t0][vi];
    dhelper.orbit2();

    bool on_boundary = dhelper.onBoundary();
    if (!on_boundary) {
      /* Go through t1: */
      dhelper.orbit1();
      t1 = dhelper.t();
      vi = dhelper.vi();
      tt_list[3] = TT_[t1][vi];
      if (use_TTi_) tti_list[3] = TTi_[t1][vi];
      dhelper.orbit2();
      vi = dhelper.vi();
      tt_list[2] = TT_[t1][vi];
      if (use_TTi_) tti_list[2] = TTi_[t1][vi];
      dhelper.orbit2();
      vi = dhelper.vi();
      v3 = TV_[t1][vi];
    } else {
      v3 = -1;
      tt_list[2] = -1;
      tt_list[3] = -1;
      if (use_TTi_) {
	tti_list[2] = -1;
	tti_list[3] = -1;
      }
    }

    /* Step 2: Overwrite one/two triangles, create two/four new. */
    /* t0 = t0; */
    if (on_boundary) {
      t1 = nT_;
      check_capacity(0,nT_+1);
      t2 = -1;
      t3 = -1;
    } else {
      /* t1 = t1; */
      t2 = nT_;
      t3 = nT_+1;
      check_capacity(0,nT_+2);
    }
    /* t0 */
    t = t0;
    TV_[t][0] = v;
    TV_[t][1] = v1;
    TV_[t][2] = v0;
    TT_[t][0] = tt_list[0];
    TT_[t][1] = t3;
    TT_[t][2] = t1;
    if (use_TTi_) {
      TTi_[t][0] = tti_list[0];
      TTi_[t][1] = 2;
      TTi_[t][2] = 1;
    }
    /* t1 */
    t = t1;
    TV_[t][0] = v;
    TV_[t][1] = v2;
    TV_[t][2] = v1;
    TT_[t][0] = tt_list[1];
    TT_[t][1] = t0;
    TT_[t][2] = t2;
    if (use_TTi_) {
      TTi_[t][0] = tti_list[1];
      TTi_[t][1] = 2;
      TTi_[t][2] = 1;
    }
    if (!on_boundary) {
      /* t2 */
      t = t2;
      TV_[t][0] = v;
      TV_[t][1] = v3;
      TV_[t][2] = v2;
      TT_[t][0] = tt_list[2];
      TT_[t][1] = t1;
      TT_[t][2] = t3;
      if (use_TTi_) {
	TTi_[t][0] = tti_list[2];
	TTi_[t][1] = 2;
	TTi_[t][2] = 1;
      }
      /* t3 */
      t = t3;
      TV_[t][0] = v;
      TV_[t][1] = v0;
      TV_[t][2] = v3;
      TT_[t][0] = tt_list[3];
      TT_[t][1] = t2;
      TT_[t][2] = t0;
      if (use_TTi_) {
	TTi_[t][0] = tti_list[3];
	TTi_[t][1] = 2;
	TTi_[t][2] = 1;
      }
    }

    /* Step 3: Relink neighbouring triangles. */
    if (use_TTi_) {
      if (TT_[t0][0]>=0) TTi_[TT_[t0][0]][TTi_[t0][0]] = 0;
      if (TT_[t1][0]>=0) TTi_[TT_[t1][0]][TTi_[t1][0]] = 0;
      if (TT_[t0][0]>=0) TT_[TT_[t0][0]][TTi_[t0][0]] = t0;
      if (TT_[t1][0]>=0) TT_[TT_[t1][0]][TTi_[t1][0]] = t1;
      if (!on_boundary) {
	if (TT_[t2][0]>=0) TTi_[TT_[t2][0]][TTi_[t2][0]] = 0;
	if (TT_[t3][0]>=0) TTi_[TT_[t3][0]][TTi_[t3][0]] = 0;
	if (TT_[t2][0]>=0) TT_[TT_[t2][0]][TTi_[t2][0]] = t2;
	if (TT_[t3][0]>=0) TT_[TT_[t3][0]][TTi_[t3][0]] = t3;
      }
    } else {
      if (TT_[t0][0]>=0) {
	dhelper = Dart(*this,t0,1,1).orbit0rev();
	dhelper.orbit2();
	TT_[dhelper.t()][dhelper.vi()] = t0;
      }
      if (TT_[t1][0]>=0) {
	dhelper = Dart(*this,t1,1,1).orbit0rev();
	dhelper.orbit2();
	TT_[dhelper.t()][dhelper.vi()] = t1;
      }
      if (!on_boundary) {
	if (TT_[t2][0]>=0) {
	  dhelper = Dart(*this,t2,1,1).orbit0rev();
	  dhelper.orbit2();
	  TT_[dhelper.t()][dhelper.vi()] = t2;
	}
	if (TT_[t3][0]>=0) {
	  dhelper = Dart(*this,t3,1,1).orbit0rev();
	  dhelper.orbit2();
	  TT_[dhelper.t()][dhelper.vi()] = t3;
	}
      }
    }

    /* Step 4: Update triangle count. */
    if (on_boundary)
      nT_ = nT_+1;
    else
      nT_ = nT_+2;
  
    /* Link vertices to triangles */
    if (use_VT_) {
      if (!on_boundary) {
	setVTtri(t3);
	setVTtri(t2);
      }
      setVTtri(t1);
      setVTtri(t0);
    }

    /* Debug code: */
    /*
    std::cout << "TT is \n" << TTO();
    rebuildTT();
    std::cout << "TT should be \n" << TTO();
    if (use_TTi_) {
      std::cout << "TTi is \n" << TTiO();
      rebuildTTi();
      std::cout << "TTi should be \n" << TTiO();
    }
    */

    redrawX11("Edge split");
    
    return Dart(*this,t1,1,0);
  }

  /*!
     \verbatim
      2          2
     |  \       |\ \
     |   \      |2\1\
     |    1 --> | v--1
     |   /      | d0/
     | d/       |/ /
      0          0
     \endverbatim
   
     Dart 0-1 --> v-1
  */
  Dart Mesh::splitTriangle(const Dart& d, int v)
  {
    Dart dhelper = d;
    int t, vi;
    int v0, v1, v2;
    int t0, t1, t2;
    int tt_list[3];
    int tti_list[3];
    if (d.edir()<0) dhelper.alpha1(); /* Correct dart orientation */

    /* Step 1: Store geometry information. */
    t = dhelper.t();
    vi = dhelper.vi();
    v0 = TV_[t][vi];
    tt_list[1] = TT_[t][vi];
    if (use_TTi_) tti_list[1] = TTi_[t][vi];
    dhelper.orbit2();
    t = dhelper.t();
    vi = dhelper.vi();
    v1 = TV_[t][vi];
    tt_list[2] = TT_[t][vi];
    if (use_TTi_) tti_list[2] = TTi_[t][vi];
    dhelper.orbit2();
    t = dhelper.t();
    vi = dhelper.vi();
    v2 = TV_[t][vi];
    tt_list[0] = TT_[t][vi];
    if (use_TTi_) tti_list[0] = TTi_[t][vi];
    dhelper.orbit2();

    /* Step 2: Overwrite one triangle, create two new. */
    t0 = t;
    t1 = nT_;
    t2 = nT_+1;
    check_capacity(0,nT_+2);
    TV_[t0][0] = v;
    TV_[t0][1] = v0;
    TV_[t0][2] = v1;
    TT_[t0][0] = tt_list[0];
    TT_[t0][1] = t1;
    TT_[t0][2] = t2;
    if (use_TTi_) {
      TTi_[t0][0] = tti_list[0];
      TTi_[t0][1] = 2;
      TTi_[t0][2] = 1;
    }
    TV_[t1][0] = v;
    TV_[t1][1] = v1;
    TV_[t1][2] = v2;
    TT_[t1][0] = tt_list[1];
    TT_[t1][1] = t2;
    TT_[t1][2] = t0;
    if (use_TTi_) {
      TTi_[t1][0] = tti_list[1];
      TTi_[t1][1] = 2;
      TTi_[t1][2] = 1;
    }
    TV_[t2][0] = v;
    TV_[t2][1] = v2;
    TV_[t2][2] = v0;
    TT_[t2][0] = tt_list[2];
    TT_[t2][1] = t0;
    TT_[t2][2] = t1;
    if (use_TTi_) {
      TTi_[t2][0] = tti_list[2];
      TTi_[t2][1] = 2;
      TTi_[t2][2] = 1;
    }

    /* Step 3: Relink neighbouring triangles. */
    if (use_TTi_) {
      if (TT_[t0][0]>=0) TTi_[TT_[t0][0]][TTi_[t0][0]] = 0;
      if (TT_[t1][0]>=0) TTi_[TT_[t1][0]][TTi_[t1][0]] = 0;
      if (TT_[t2][0]>=0) TTi_[TT_[t2][0]][TTi_[t2][0]] = 0;
      if (TT_[t0][0]>=0) TT_[TT_[t0][0]][TTi_[t0][0]] = t0;
      if (TT_[t1][0]>=0) TT_[TT_[t1][0]][TTi_[t1][0]] = t1;
      if (TT_[t2][0]>=0) TT_[TT_[t2][0]][TTi_[t2][0]] = t2;
    } else {
      if (TT_[t0][0]>=0) {
	dhelper = Dart(*this,t0,1,1).orbit0rev();
	dhelper.orbit2();
	TT_[dhelper.t()][dhelper.vi()] = t0;
      }
      if (TT_[t1][0]>=0) {
	dhelper = Dart(*this,t1,1,1).orbit0rev();
	dhelper.orbit2();
	TT_[dhelper.t()][dhelper.vi()] = t1;
      }
      if (TT_[t2][0]>=0) {
	dhelper = Dart(*this,t2,1,1).orbit0rev();
	dhelper.orbit2();
	TT_[dhelper.t()][dhelper.vi()] = t2;
      }
    }

    /* Step 4: Update triangle count. */
    nT_ = nT_+2;

    /* Link vertices to triangles */
    if (use_VT_) {
      setVTtri(t2);
      setVTtri(t1);
      setVTtri(t0);
    }

    /* Debug code: */
    /*
    std::cout << "TT is \n" << TTO();
    rebuildTT();
    std::cout << "TT should be \n" << TTO();
    if (use_TTi_) {
      std::cout << "TTi is \n" << TTiO();
      rebuildTTi();
      std::cout << "TTi should be \n" << TTiO();
    }
    */
    
    redrawX11("Triangle split");

    return Dart(*this,t0,1,0);
  }





  std::ostream& operator<<(std::ostream& output, const Mesh& M)
  {
    //    output << "S =\n" << M.SO();
    output << "TV =\n" << M.TVO();
    output << "TT =\n" << M.TTO();
    if (M.useVT())
      output << "VT =\n" << M.VTO();
    if (M.useTTi())
      output << "TTi =\n" << M.TTiO();
    return output;
  }


  M3intO Mesh::TVO() const { return M3intO(TV_,nT_); };
  M3intO Mesh::TTO() const { return M3intO(TT_,nT_); };
  MintO Mesh::VTO() const { return MintO(VT_,nV_); };
  M3intO Mesh::TTiO() const { return M3intO(TTi_,nT_); };
  M3doubleO Mesh::SO() const { return M3doubleO(S_,nV_); };

  std::ostream& operator<<(std::ostream& output, const MintO& MO)
  {
    if (!MO.M_) return output;
    for (int i = 0; i < (int)MO.n_; i++) {
      output << ' ' << std::right << std::setw(4)
	     << MO.M_[i];
    }
    std::cout << std::endl;
    return output;
  }

  std::ostream& operator<<(std::ostream& output, const M3intO& MO)
  {
    if (!MO.M_) return output;
    for (int j = 0; j<3; j++) {
      for (int i = 0; i < (int)MO.n_; i++) {
	output << ' ' << std::right << std::setw(4)
	       << MO.M_[i][j];
      }
      std::cout << std::endl;
    }
    return output;
  }

  std::ostream& operator<<(std::ostream& output, const M3doubleO& MO)
  {
    if (!MO.M_) return output;
    for (int i = 0; i < (int)MO.n_; i++) {
      for (int j = 0; j<3; j++)
	output << ' ' << std::right << std::setw(10) << std::scientific
	       << MO.M_[i][j];
      std::cout << std::endl;
    }
    return output;
  }






  std::ostream& operator<<(std::ostream& output, const Dart& d)
  {
    output << std::right << std::setw(1) << d.t_
	   << std::right << std::setw(3) << d.edir_
	   << std::right << std::setw(2) << d.vi_;
    if ((!d.isnull()) && (d.t_<(int)d.M()->nT())) {
      output << " ("
	     << d.M()->TV()[d.t_][d.vi_]
	     << ","
	     << d.M()->TV()[d.t_][(d.vi_+(3+d.edir_))%3]
	     << ")";
    }
      
    return output;
  }


  Dart& Dart::alpha0()
  {
    vi_ = (vi_ + (3+edir_) ) % 3;
    edir_ = -edir_;
    return *this;
  }

  Dart& Dart::alpha1()
  {
    edir_ = -edir_;
    return *this;
  }

  Dart& Dart::alpha2()
  {
    if (!M_->use_TTi_) {
      int vi;
      int v = M_->TV_[t_][vi_];
      int t = M_->TT_[t_][(vi_+(3-edir_))%3];
      if (t<0) return *this;
      for (vi = 0; (vi<3) && (M_->TV_[t][vi] != v); vi++) { }
      if (vi>=3) return *this; /* Error! This should never happen! */
      vi_ = vi;
      edir_ = -edir_;
      t_ = t;
    } else {
      int vi = (vi_+(3-edir_))%3;
      int t = M_->TT_[t_][vi];
      if (t<0) return *this;
      vi_ = (M_->TTi_[t_][vi]+(3-edir_))%3;
      edir_ = -edir_;
      t_ = t;
    }
    return *this;
  }

  Dart& Dart::orbit0()
  {
    int t = t_;
    alpha1();
    alpha2();
    if (t == t_) alpha1(); /* Undo; boundary. */
    return *this;
  }

  Dart& Dart::orbit1()
  {
    int t = t_;
    alpha2();
    if (t != t_) alpha0(); /* Do only if not at boundary. */
    return *this;
  }

  Dart& Dart::orbit2()
  {
    /* "alpha0(); alpha1();" would be less efficient. */
    vi_ = (vi_+(3+edir_))%3;
    return *this;
  }

  Dart& Dart::orbit0rev()
  {
    int t = t_;
    alpha2();
    if (t != t_) alpha1(); /* Do only if not at boundary. */
    return *this;
  }

  Dart& Dart::orbit1rev() /* Equivalent to orbit1() */
  {
    orbit1();
    return *this;
  }

  Dart& Dart::orbit2rev()
  {
    /* "alpha1(); alpha0();" would be less efficient. */
    vi_ = (vi_+(3-edir_))%3;
    return *this;
  }




  /*!
    Alg 9.1

    If the point is located within the triangulation domain,
    delta_min and the returned Dart correspond to the triangle edge
    with smallest distance, as measured by inLeftHalfspace.

    If the point is not found, a null Dart is returned.
   */
  Dart Mesh::locatePoint(const Dart& d0,
			 const Point s,
			 double* delta_min) const
  {
    Dart dart;
    if (d0.isnull())
      dart = Dart(*this,0);
    else
      dart = Dart(*this,d0.t(),1,d0.vi());
    Dart dart_start = dart;
    double delta;
    Dart dart_min = Dart();
    while (1) {
      std::cout << dart_start << ' '
		<< dart << ' '
		<< inLeftHalfspace(dart,s)
		<< std::endl;
      delta = inLeftHalfspace(dart,s);
      if (dart_min.isnull() || (delta<*delta_min)) {
	dart_min = dart;
	*delta_min = delta;
      }
      if (delta >= -MESH_EPSILON) {
	dart.orbit2();
	if (dart==dart_start)
	  return dart_min;
      } else {
	if (dart.onBoundary())
	  return Dart();
	dart.alpha2();
	dart_start = dart;
	dart_start.alpha0();
	dart.alpha1();
	dart_min = dart_start;
	*delta_min = -delta;
      }
    }

    return Dart();
  }

  /*!
    Alg 9.1 modified to locate a pre-existing vertex.

    If the vertex is not found, a null Dart is returned.
   */
  Dart Mesh::locateVertex(const Dart& d0,
			  const int v) const
  {
    if (use_VT_) {
      int t;
      t = VT_[v];
      if (t<0) /* Vertex not connected to any triangles. */
	return Dart();
      if (TV_[t][0] == v)
	return Dart(*this,t,1,0);
      if (TV_[t][1] == v)
	return Dart(*this,t,1,1);
      if (TV_[t][2] == v)
	return Dart(*this,t,1,2);
      return Dart(); /* ERROR: Inconsistent data structures! */
    }

    int i;
    Dart dart;
    if (d0.isnull())
      dart = Dart(*this,0);
    else
      dart = Dart(*this,d0.t(),1,d0.vi());
    Dart dart_start = dart;
    double delta;
    Dart dart_min = Dart();
    double* s = &(S_[v][0]);
    double delta_min = 0.0;
    while (1) {
      std::cout << dart_start << ' '
		<< dart << ' '
		<< inLeftHalfspace(dart,s)
		<< std::endl;
      for (i=0;i<3;i++) {
	if (TV_[dart.t()][dart.vi()] == v)
	  return dart;
	dart.orbit2();
      }

      delta = inLeftHalfspace(dart,s);
      if (dart_min.isnull() || (delta<delta_min)) {
	dart_min = dart;
	delta_min = delta;
      }
      if (delta >= -MESH_EPSILON) {
	dart.orbit2();
	if (dart==dart_start) {
	  for (i=0;i<3;i++) {
	    if (TV_[dart.t()][dart.vi()] == v)
	      return dart;
	    dart.orbit2();
	  }
	  return Dart(); /* ERROR: Point located, but not the vertex itself. */
	}
      } else {
	if (dart.onBoundary())
	  return Dart();
	dart.alpha2();
	dart_start = dart;
	dart_start.alpha0();
	dart.alpha1();
	dart_min = dart_start;
	delta_min = -delta;
      }
    }

    return Dart();
  }



  /*! Alg 4.3 */
  bool MeshC::recSwapDelaunay(const Dart& d0)
  {
    Dart d1, d2;

    std::cout << "Trying to swap " << d0 << std::endl;

    if (d0.isnull() or d0.onBoundary())
      return true; /* OK. Not allowed to swap. */
    if (isSegment(d0))
      return true ; /* OK. Not allowed to swap. */
    if (M_->circumcircleOK(d0))
      return true; /* OK. Need not swap. */

    std::cout << "Swap " << d0 << std::endl;

    /* Get opposing darts. */
    d1 = d0;
    d1.alpha1();
    if (d1.onBoundary()) d1 = Dart(); else d1.alpha2();
    d2 = d0;
    d2.orbit2rev().alpha1(); 
    if (d2.onBoundary()) d2 = Dart(); else d2.alpha2();
    
    std::cout << "TVpre  = " << std::endl << M_->TVO();
    swapEdge(d0);
    std::cout << "TVpost = " << std::endl << M_->TVO();
    std::cout << "TTpost = " << std::endl << M_->TTO();

    if (!d1.isnull()) recSwapDelaunay(d1);
    if (!d2.isnull()) recSwapDelaunay(d2);
    return true;
  }


  /*! Alg 9.3 */
  Dart MeshC::splitTriangleDelaunay(const Dart& td, int v)
  {
    Dart d, d0, d1, d2;

    if (td.isnull()) { return Dart(); }; /* ERROR */
    /* Get opposing darts. */
    d = td;
    if (d.onBoundary()) d0 = Dart(); else {d0 = d; d0.orbit1();} 
    d.orbit2();
    if (d.onBoundary()) d1 = Dart(); else {d1 = d; d1.orbit1();} 
    d.orbit2();
    if (d.onBoundary()) d2 = Dart(); else {d2 = d; d2.orbit1();} 

    std::cout << "TV = " << std::endl << M_->TVO();
    std::cout << "Split triangle with vertex " << v << std::endl;
    d = splitTriangle(td,v);
    
    if (!d0.isnull()) recSwapDelaunay(d0);
    if (!d1.isnull()) recSwapDelaunay(d1);
    if (!d2.isnull()) recSwapDelaunay(d2);

    std::cout << "TV = " << std::endl << M_->TVO();
    
    return d;
  }

  /*! Modified Alg 9.3 */
  Dart MeshC::splitEdgeDelaunay(const Dart& ed, int v)
  {
    Dart d, d0, d1, d2, d3;

    if (ed.isnull()) { return Dart(); }; /* ERROR */
    /* Get opposing darts. */
    d = ed;
    d.orbit2();
    if (d.onBoundary()) d0 = Dart(); else {d0 = d; d0.orbit1();} 
    d.orbit2();
    if (d.onBoundary()) d1 = Dart(); else {d1 = d; d1.orbit1();} 
    d = ed;
    if (d.onBoundary()) {
      d2 = Dart();
      d3 = Dart();
    } else {
      d.orbit0rev();
      if (d.onBoundary()) d2 = Dart(); else {d2 = d; d2.orbit1();} 
      d.orbit2();
      if (d.onBoundary()) d3 = Dart(); else {d3 = d; d3.orbit1();} 
    }

    std::cout << "TV = " << std::endl << M_->TVO();
    std::cout << "Split edge with vertex " << v << std::endl;
    d = splitEdge(ed,v);
    
    if (!d0.isnull()) recSwapDelaunay(d0);
    if (!d1.isnull()) recSwapDelaunay(d1);
    if (!d2.isnull()) recSwapDelaunay(d2);
    if (!d3.isnull()) recSwapDelaunay(d3);

    std::cout << "TV = " << std::endl << M_->TVO();
    
    return d;
  }

  /*! Alg 9.3 */
  bool MeshC::insertNode(int v, const Dart& ed)
  {
    Dart td;
    double delta;

    std::cout << "Locating node " << v << std::endl;
    td = M_->locatePoint(ed,M_->S()[v],&delta);
    if (td.isnull()) { return false; }; /* ERROR, not found! */
    std::cout << "Closest dart " << td
	      << ' ' << delta << std::endl;

    if (delta>10*MESH_EPSILON) { /* Split triangle */
      splitTriangleDelaunay(td,v);
    } else { /* Split edge */
      splitEdgeDelaunay(td,v);
    }

    return true;
  }

  bool MeshC::DT(const vertexListT& v_set)
  {
    if (is_pruned_) 
      return false; /* ERROR, cannot safely insert nodes into a pruned
		       triangulation. Call insertNode directly if known to
		       be visible/reachable from a given edge.  */

    if (state_ < State_CHT)
      return false; /* TODO: Add convex enclosure? */

    if (state_ < State_DT)
      if (!prepareDT()) /* Make sure we have a DT. */
	return false;

    if (state_>=State_CDT)
      std::cout << "Boundary segments before DT:" << std::endl << boundary_;

    int v;
    vertexListT::const_iterator v_iter;
    Dart td, d, d0, d1, d2;

    for (v_iter = v_set.begin(); v_iter != v_set.end(); v_iter++) {
      v = *v_iter;
      insertNode(v,Dart(*M_,0)); /* TODO: More clever starting edge? */
      std::cout << M_->VTO();

      if (state_>=State_CDT)
	std::cout << "Boundary segments after DT:" << std::endl << boundary_;

    }
      
    //    xtmpl_press_ret("nodes inserted");

    state_ = State_DT;
    return true;
  }


  bool MeshC::prepareDT()
  {
    if (state_<State_DT) {
      /* We need to build a DT first. */
      triangleListT t_set;
      for (int t=0;t<(int)M_->nT();t++)
	t_set.push_back(t);
      if (LOP(t_set))
	state_ = State_DT;
    }
    return (state_>=State_DT) && (!is_pruned_);
  }


  bool MeshC::prepareCDT()
  {
    if (!prepareDT()) return false; /* Make sure we have a DT. */
    if (state_>=State_CDT)
      return true; /* Nothing to do. Data structures already active. */

    const int* tt;
    int vi;
    Dart d;
    for (int t=0;t<(int)M_->nT();t++) {
      tt = M_->TT()[t];
      for (vi=0;vi<3;vi++)
	if (tt[vi]<0) {
	  d = Dart(*M_,t,1,(vi+1)%3);
	  boundary_.insert(d);
	}
    }

    state_ = State_CDT;
    return true;
  }

  bool MeshC::prepareRCDT(double skinny_limit, double big_limit)
  {
    if (!prepareCDT()) return false; /* Make sure we have a CDT. */

    skinny_.clear();
    big_.clear();
    skinny_.setQ(skinny_limit);
    big_.setQ(big_limit);

    for (int t=0;t<(int)M_->nT();t++) {
      skinny_.insert(Dart(*M_,t));
      big_.insert(Dart(*M_,t));
    }

    state_ = State_RCDT;
    return true;
  }



  bool MeshC::CDTBoundary(const constrListT& constr)
  {
    if (!prepareCDT()) return false;

    constr_boundary_ = constrListT(constr.begin(),constr.end());
    
    return buildCDT();
  };

  bool MeshC::CDTInterior(const constrListT& constr)
  {
    if (!prepareCDT()) return false;

    constr_interior_ = constrListT(constr.begin(),constr.end());
    
    return buildCDT();
  };






  bool MeshC::LOP(const triangleListT& t_set)
  {
    /* TODO: Implement. */
    NOT_IMPLEMENTED;

    return true;
  }


  bool MeshC::buildCDT()
  {
    if (!prepareCDT()) return false;

    /* TODO: Implement. */

    constrListT::iterator ci_next;
    for (constrListT::iterator ci = constr_boundary_.begin();
	 ci != constr_boundary_.end(); ) {
      NOT_IMPLEMENTED;
      if (true) {
	ci_next = ci;
	ci_next++;
	ci = constr_boundary_.erase(ci);
	ci = ci_next;
      } else
	ci++;
    }
    for (constrListT::iterator ci = constr_interior_.begin();
	 ci != constr_interior_.end(); ) {
      NOT_IMPLEMENTED;
      if (true) {
	ci_next = ci;
	ci_next++;
	ci = constr_interior_.erase(ci);
	ci = ci_next;
      } else
	ci++;
    }

    std::cout << "Boundary segments after CDT:" << std::endl << boundary_;
    std::cout << "Interior segments after CDT:" << std::endl << interior_;

    return (constr_boundary_.empty() && constr_interior_.empty());
  };

  bool MeshC::buildRCDT()
  {
    if (state_<State_RCDT)
      return false; /* ERROR: RCDT not initialised. */

    std::cout << "Encroached boundary segments before RCDT:" << std::endl
	      << boundary_;
    std::cout << "Encroached interior segments before RCDT:" << std::endl
	      << interior_;
    std::cout << "Skinny triangles before RCDT:" << std::endl << skinny_;
    std::cout << "Big triangles before RCDT:" << std::endl << big_;

    /* TODO: Implement. */
    NOT_IMPLEMENTED;

    return true;
  };

  bool MeshC::RCDT(double skinny_limit, double big_limit)
  {
    if (!prepareRCDT(skinny_limit,big_limit)) return false;
    return buildRCDT();
  };


  bool MeshC::PruneExterior()
  {
    if (state_ < State_CDT) {
      /* Since there are no constraints at this state, no exterior
	 needs to be pruned, but add the boundary to the constraint
	 set anyway, to get to state State_CDT. */
      prepareCDT();
      is_pruned_ = true;
      return true;
    }
    is_pruned_ = true;
    
    /* TODO: Implement. */
    NOT_IMPLEMENTED;

    return true;
  };





  Dart MeshC::swapEdge(const Dart& d)
  {
    if (state_ < State_CDT) {
      return M_->swapEdge(d);
    }

    if (boundary_.segm(d) || interior_.segm(d)) {
      /* ERROR: Not allowed to swap. */
      std::cout << "ERROR: Not allowed to swap dart " << std::endl
		<< d << std::endl;
      return d;
    }

    /* Collect CDT data */
    bool segm_b[4];
    bool segm_i[4];
    Dart dh(d);
    dh.orbit2rev();
    if ((segm_b[1] = boundary_.found(dh))) boundary_.erase(dh);
    if ((segm_i[1] = interior_.found(dh))) interior_.erase(dh);
    dh.orbit2rev();
    if ((segm_b[2] = boundary_.found(dh))) boundary_.erase(dh);
    if ((segm_i[2] = interior_.found(dh))) interior_.erase(dh);
    dh.orbit0().orbit2rev();
    if ((segm_b[3] = boundary_.found(dh))) boundary_.erase(dh);
    if ((segm_i[3] = interior_.found(dh))) interior_.erase(dh);
    dh.orbit2rev();
    if ((segm_b[0] = boundary_.found(dh))) boundary_.erase(dh);
    if ((segm_i[0] = interior_.found(dh))) interior_.erase(dh);

    if (state_>=State_RCDT) {
      /* TODO: Collect RCDT data */
      NOT_IMPLEMENTED;
    }

    Dart dnew(M_->swapEdge(d));

    /* Reassemble CDT data */
    dh = dnew;
    dh.orbit2();
    if (segm_b[1]) boundary_.insert(dh);
    if (segm_i[1]) interior_.insert(dh);
    dh.orbit2();
    if (segm_b[0]) boundary_.insert(dh);
    if (segm_i[0]) interior_.insert(dh);
    dh.orbit2().orbit0rev();
    if (segm_b[3]) boundary_.insert(dh);
    if (segm_i[3]) interior_.insert(dh);
    dh.orbit2();
    if (segm_b[2]) boundary_.insert(dh);
    if (segm_i[2]) interior_.insert(dh);

    if (state_>=State_RCDT) {
      /* TODO: Reassemble RCDT data */
      NOT_IMPLEMENTED;
    }

    std::cout << "Edge swapped, boundary segments:" << std::endl
	      << boundary_;

    return dnew;
  }

  Dart MeshC::splitEdge(const Dart& d, int v)
  {
    if (state_ < State_CDT) {
      return M_->splitEdge(d,v);
    }

    /* Collect CDT data */
    Dart dh(d);
    bool segm_b[6];
    bool segm_i[6];
    for (int i=0;i<3;i++) {
      if ((segm_b[i] = boundary_.found(dh))) boundary_.erase(dh);
      if ((segm_i[i] = interior_.found(dh))) interior_.erase(dh);
      dh.orbit2();
    }
    if (!dh.onBoundary()) {
      dh.orbit1();
      for (int i=3;i<6;i++) {
	if ((segm_b[i] = boundary_.found(dh))) boundary_.erase(dh);
	if ((segm_i[i] = interior_.found(dh))) interior_.erase(dh);
	dh.orbit2();
      }
    }

    if (state_>=State_RCDT) {
      /* TODO: Collect RCDT data */
      NOT_IMPLEMENTED;
    }

    Dart dnew(M_->splitEdge(d,v));

    /* Reassemble CDT data */
    dh = dnew;
    if (segm_b[0]) boundary_.insert(dh);
    if (segm_i[0]) interior_.insert(dh);
    dh.orbit2();
    if (segm_b[1]) boundary_.insert(dh);
    if (segm_i[1]) interior_.insert(dh);
    dh.orbit2().orbit0rev();
    if (segm_b[2]) boundary_.insert(dh);
    if (segm_i[2]) interior_.insert(dh);
    dh.orbit2();
    if (segm_b[0]) boundary_.insert(dh);
    if (segm_i[0]) interior_.insert(dh);
    if (!dh.onBoundary()) {
      dh.orbit1();
      if (segm_b[3]) boundary_.insert(dh);
      if (segm_i[3]) interior_.insert(dh);
      dh.orbit2();
      if (segm_b[4]) boundary_.insert(dh);
      if (segm_i[4]) interior_.insert(dh);
      dh.orbit2().orbit0rev();
      if (segm_b[5]) boundary_.insert(dh);
      if (segm_i[5]) interior_.insert(dh);
      dh.orbit2();
      if (segm_b[3]) boundary_.insert(dh);
      if (segm_i[3]) interior_.insert(dh);
    }

    if (state_>=State_RCDT) {
      /* TODO: Reassemble RCDT data */
      NOT_IMPLEMENTED;
    }

    std::cout << "Edge split, boundary segments:" << std::endl
	      << boundary_;

    xtmpl_press_ret("Edge has been split");

    return dnew;
  }

  Dart MeshC::splitTriangle(const Dart& d, int v)
  {
    if (state_ < State_CDT) {
      return M_->splitTriangle(d,v);
    }

    /* Collect CDT data */
    Dart dh(d);
    bool segm_b[3];
    bool segm_i[3];
    for (int i=0;i<3;i++) {
      if ((segm_b[i] = boundary_.found(dh))) boundary_.erase(dh);
      if ((segm_i[i] = interior_.found(dh))) interior_.erase(dh);
      dh.orbit2();
    }

    if (state_>=State_RCDT) {
      /* TODO: Collect RCDT data */
      NOT_IMPLEMENTED;
    }

    Dart dnew(M_->splitTriangle(d,v));

    /* Reassebmle CDT data */
    dh = dnew;
    for (int i=0;i<3;i++) {
      dh.orbit2();
      if (segm_b[i]) boundary_.insert(dh);
      if (segm_i[i]) interior_.insert(dh);
      dh.orbit2rev().orbit0();
    }

    if (state_>=State_RCDT) {
      /* TODO: Reassemble RCDT data */
      NOT_IMPLEMENTED;
    }

    std::cout << "Triangle split, boundary segments:" << std::endl
	      << boundary_;

    return dnew;
  }




  bool MeshC::isSegment(const Dart& d) const
  {
    if (state_<State_CDT) /* No segments */
      return false;

    return (boundary_.segm(d) || interior_.segm(d));
  }

































  bool MCQ::found(const Dart& d) const
  {
    return (darts_.find(d) != darts_.end());
  }

  bool MCQ::foundQ(const Dart& d) const
  {
    map_type::const_iterator i = darts_.find(d);
    if (i == darts_.end())
      return false;
    return (darts_quality_.find(MCQdv(i->first,i->second)) !=
	    darts_quality_.end());
  }

  const double MCQ::quality(const Dart& d) const
  {
    if (empty())
      return 0.0;
    return darts_.find(d)->second;
  }

  Dart MCQ::quality() const
  {
    if (emptyQ())
      return Dart();
    return darts_quality_.begin()->d_;
  }

  void MCQ::insert(const Dart& d)
  {
    double quality_ = calcQ(d);
    if (quality_>0.0) {
      darts_.insert(map_key_type(d,quality_));
      darts_quality_.insert(MCQdv(d,quality_));
    } else if (!only_quality_)
      darts_.insert(map_key_type(d,quality_));
  }

  void MCQ::erase(const Dart& d)
  {
    double quality_;
    map_type::iterator i = darts_.find(d);
    if (i != darts_.end()) {
      quality_ = i->second;
      darts_.erase(i);
      set_type::iterator j = darts_quality_.find(MCQdv(d,quality_));
      if (j != darts_quality_.end())
	darts_quality_.erase(j);
    }
  }

  std::ostream& operator<<(std::ostream& output, const MCQ& Q)
  {
    if (Q.empty()) return output;
    output << "N,n = " << Q.count() << "," << Q.countQ() << std::endl;
    for (MCQ::map_type::const_iterator qi = Q.darts_.begin();
	 qi != Q.darts_.end(); qi++) {
      output << ' ' << qi->first
	     << ' ' << std::scientific << qi->second
	     << ' ' << Q.foundQ(qi->first)
	     << std::endl;
    }
    return output;
  }






  triMCQ::triMCQ(bool only_quality, double quality_limit)
    : MCQ(only_quality), quality_limit_(quality_limit)
  {
    setQ(quality_limit);
  }

  void triMCQ::setQ(double quality_limit)
  {
    quality_limit_ = quality_limit;
    if (!empty()) {
      /* TODO: Implement updating the sets. */
      NOT_IMPLEMENTED;
    }
  }

  double triMCQ::calcQ(const Dart& d) const {
    double quality_lim_ = quality_limit_; /* TODO: min_vi ql[t][vi] */
    return (calcQtri(d) - quality_lim_);
  };

  double skinnyMCQ::calcQtri(const Dart& d) const
  {
    double quality_ = d.M()->skinnyQuality(d.t());
    return quality_;
  }
  
  double bigMCQ::calcQtri(const Dart& d) const
  {
    double quality_ = d.M()->bigQuality(d.t());
    return quality_;
  }
  
  double segmMCQ::calcQ(const Dart& d) const
  {
    double quality_ = d.M()->encroachedQuality(d);
    Dart dhelper(d);
    dhelper.orbit1();
    if (d.t() != dhelper.t()) {
      double quality1_ = d.M()->encroachedQuality(dhelper);
      if (quality1_>quality_)
	quality_ = quality1_;
    }
    return (quality_-encroached_limit_);
  }

  bool segmMCQ::segm(const Dart& d) const
  {
    if (found(d)) return true;
    Dart dhelper(d);
    dhelper.orbit1();
    return ((dhelper.t() != d.t()) && found(dhelper));
  }









  void Xtmpl::arc(const double* s0, const double* s1)
  {
    int n = 10;
    xtmpl_window = window_;
    double p0[2];
    double p1[2];
    double s[3];
    double l;
    int dim;
    p1[0] = s0[0];
    p1[1] = s0[1];
    for (int i=1;i<=n;i++) {
      l = 0.0;
      p0[0] = p1[0]; p0[1] = p1[1];
      for (dim=0;dim<3;dim++) {
	s[dim] = ((n-i)*s0[dim]+i*s1[dim])/n;
	l += s[dim]*s[dim];
      }
      l = std::sqrt(l);
      for (int dim=0;dim<2;dim++)
	p1[dim] = s[dim]/l;
      
      xtmpl_draw_line((int)(sx_*(p0[0]-minx_)/(maxx_-minx_)),
		      (int)(sy_*(p0[1]-miny_)/(maxy_-miny_)),
		      (int)(sx_*(p1[0]-minx_)/(maxx_-minx_)),
		      (int)(sy_*(p1[1]-miny_)/(maxy_-miny_)));
    }
  };
  void Xtmpl::line(const double* s0, const double* s1)
  {
    xtmpl_window = window_;
    xtmpl_draw_line((int)(sx_*(s0[0]-minx_)/(maxx_-minx_)),
		    (int)(sy_*(s0[1]-miny_)/(maxy_-miny_)),
		    (int)(sx_*(s1[0]-minx_)/(maxx_-minx_)),
		    (int)(sy_*(s1[1]-miny_)/(maxy_-miny_)));
  };
  void Xtmpl::text(const double* s0, std::string str)
  {
    char* str_ = new char[str.length()+1];
    str.copy(str_,str.length(),0);
    str_[str.length()] = '\0';
    xtmpl_window = window_;
    xtmpl_text((int)(sx_*(s0[0]-minx_)/(maxx_-minx_)),
	       (int)(sy_*(s0[1]-miny_)/(maxy_-miny_)),
	       str_,str.length());
    delete[] str_;
  };
  
  


} /* namespace fmesh */
