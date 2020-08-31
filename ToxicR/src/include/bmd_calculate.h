#ifndef bmd_calculateH
#define bmd_calculateH

//#include "stdafx.h"
#include <chrono>
#include <cmath>
#ifndef WIN32
#include <cfloat>
#endif

#ifdef R_COMPILATION
    //necessary things to run in R    
    #include <RcppEigen.h>
    #include <RcppGSL.h>
#else 
    #include <Eigen/Dense>

#endif

#include <gsl/gsl_errno.h>
#include <gsl/gsl_spline.h>
#include <gsl/gsl_randist.h>
#include <gsl/gsl_cdf.h>

#include "normal_EXP_NC.h"
#include "dBMDstatmod.h"
#include "cBMDstatmod.h"
#include "bmdStruct.h"
#include "continuous_clean_aux.h"

class bmd_cdf {
public:
	bmd_cdf():probs(),BMD() {
		spline_bmd_cdf = NULL;
		spline_bmd_inv = NULL;
		acc_bmd_cdf = NULL;
		acc_bmd_inv = NULL;
				
		max_BMD = 0.0; 
		min_BMD = 0.0;

		min_prob = 0.0; 
		max_prob = 0.0; 
	}; 

	bmd_cdf & operator=(const bmd_cdf &M) {
		
		probs = M.probs;
		BMD = M.BMD;

		max_BMD = M.max_BMD; min_BMD = M.min_BMD;
		max_prob = M.max_prob; min_prob = M.min_prob;
		int error = 0;

	    
		if (probs.size() == BMD.size() && BMD.size() >0) {
			acc_bmd_inv = gsl_interp_accel_alloc();
			acc_bmd_cdf = gsl_interp_accel_alloc();
			spline_bmd_inv = gsl_spline_alloc(gsl_interp_steffen, BMD.size());
			spline_bmd_cdf = gsl_spline_alloc(gsl_interp_steffen, BMD.size());
			error = gsl_spline_init(spline_bmd_inv, probs.data(), BMD.data(), BMD.size());
			
			if (error) {
			//	Rcpp::Rcout << "error: %s\n" << gsl_strerror (error) << endl; 
				spline_bmd_inv = NULL;
				acc_bmd_inv = NULL;
			}

			error = gsl_spline_init(spline_bmd_cdf, BMD.data(), probs.data(), BMD.size());
			if (error) {
			//	Rcpp::Rcout << "error: %s\n" << gsl_strerror (error) << endl; 
			//	Rcpp::Rcout << "error: %s\n" << gsl_strerror (error) << endl; 
				spline_bmd_cdf = NULL;
				acc_bmd_cdf = NULL;
			}
		}
		
		return *this; 
	}
	bmd_cdf(const bmd_cdf &M) {
		*this = M;
	}

	bmd_cdf(std::vector<double> tx, std::vector<double> ty) :probs(tx), BMD(ty) {
		int error; 

		max_prob = *max_element(probs.begin(), probs.end()); 
		min_prob = *min_element(probs.begin(), probs.end());

		max_BMD = *max_element(BMD.begin(), BMD.end());
		min_BMD = *min_element(BMD.begin(), BMD.end());
		
		if (probs.size() == BMD.size() && BMD.size() >0) {
			acc_bmd_inv = gsl_interp_accel_alloc();
			acc_bmd_cdf = gsl_interp_accel_alloc();
			spline_bmd_inv = gsl_spline_alloc(gsl_interp_steffen, BMD.size());
			spline_bmd_cdf = gsl_spline_alloc(gsl_interp_steffen, BMD.size());
			error = gsl_spline_init(spline_bmd_inv, probs.data(), BMD.data(), BMD.size());
		
			if (error) {
				//Rcpp::Rcout << "error: %s\n" << gsl_strerror (error) << endl; 
				spline_bmd_inv = NULL;
				acc_bmd_inv = NULL;
			}

			error = gsl_spline_init(spline_bmd_cdf, BMD.data(), probs.data(), BMD.size());
			if (error) {
				//Rcpp::Rcout << "error: %s\n" << gsl_strerror (error) << endl; 
				spline_bmd_cdf = NULL;
				acc_bmd_cdf = NULL;
			}
		}

	}

	~bmd_cdf() {
		if (spline_bmd_inv) {
			gsl_spline_free(spline_bmd_inv);
		}
		if (spline_bmd_cdf) {
			gsl_spline_free(spline_bmd_cdf);
		}
		if (acc_bmd_cdf) {
			gsl_interp_accel_free(acc_bmd_cdf);
		}
		if (acc_bmd_inv) {
			gsl_interp_accel_free(acc_bmd_inv);
		}
		acc_bmd_inv = NULL;
		acc_bmd_cdf = NULL;
		spline_bmd_cdf = NULL; 
		spline_bmd_inv = NULL;
	}

	double inv(double p) {
		if (spline_bmd_cdf !=NULL && acc_bmd_cdf != NULL) {
			if (p > min_prob && p < max_prob)
				return gsl_spline_eval(spline_bmd_inv, p, acc_bmd_inv);
			if (p < min_prob)
				return 0;
			if (p > max_prob)
				return std::numeric_limits<double>::infinity();
		}
		return NAN; 
	}


	double P(double dose) {
		if (spline_bmd_cdf != NULL && acc_bmd_cdf != NULL) {
			if (dose > min_BMD && dose < max_BMD)
				return gsl_spline_eval(spline_bmd_cdf, dose, acc_bmd_cdf);
			if (dose < min_BMD)
				return 0.0;
			if (dose > max_BMD)
				return 1.0; 
		}
		return NAN;
	}
	

private:
	double min_BMD; 
	double max_BMD; 

	double min_prob;
	double max_prob; 
	std::vector<double> probs; 
	std::vector<double> BMD;
	gsl_interp_accel  *acc_bmd_cdf;
	gsl_spline     *spline_bmd_cdf;
	gsl_interp_accel  *acc_bmd_inv;
	gsl_spline     *spline_bmd_inv;
};


class bmd_analysis {
public:

	bmd_cdf BMD_CDF;
	Eigen::MatrixXd MAP_ESTIMATE;
	Eigen::MatrixXd COV;
	bool isExtra;
	double BMR;
	double MAP_BMD;
	double alpha;
	double MAP;
	contbmd type; 
	std::vector<double> expected; 
	
	bmd_analysis() : MAP_ESTIMATE(), COV(), BMD_CDF() {

	}
	bmd_analysis(const bmd_analysis &M) {
		BMD_CDF = M.BMD_CDF;
		MAP_ESTIMATE = M.MAP_ESTIMATE;
		MAP = M.MAP;
		COV = M.COV;
		BMR = M.BMR;
		MAP_BMD = M.MAP_BMD;
		alpha = M.alpha;
		type = M.type; 
		expected = M.expected; 
	}

	bmd_analysis & operator=(const bmd_analysis &M) {
		BMD_CDF = M.BMD_CDF;
		MAP_ESTIMATE = M.MAP_ESTIMATE;
		MAP = M.MAP;
		COV = M.COV;
		BMR = M.BMR;
		MAP_BMD = M.MAP_BMD;
		alpha = M.alpha;
		type = M.type; 
		expected = M.expected; 
		return *this;
	}

};


Eigen::MatrixXd convertresult_to_probs(Eigen::MatrixXd data);
double find_maBMDquantile(double q, std::vector<double> probs, std::list<bmd_analysis> analyses);
double ma_cdf(double dose, std::vector<double> probs, std::list<bmd_analysis> analyses);

/**********************************************************************
 * function bmd_analysis:
 * 		LL        -    likelihood previously defined
 *      PR        -    Prior
 * 	    fixedB    -    boolean vector of which parameters are fixed
 * 		fixedV    -    vector of what value parameters are fixed too. 
 * 	    BMDType   -    The type of BMD we are searching for
 *      BMRF      -    Benchmark Response
 *      tail_prob -    The tail probability for the Hybrid approach
 *      isIncreasing - true if the BMD should be calculated for an increasing data set
 *      alpha     -    Defines the the lower and upper stopping point for integeration
 *      step_size -    The approximate step size 
 *  returns: 
 *      bmd_analysis class. 
 * *******************************************************************/
template <class LL, class PR>
bmd_analysis bmd_analysis_CNC(LL likelihood, PR prior, 
			 std::vector<bool> fixedB, std::vector<double> fixedV,
			 contbmd BMDType, double BMRF,   double tail_prob, 
			 bool isIncreasing, double alpha, double step_size,
			 Eigen::MatrixXd init = Eigen::MatrixXd::Zero(1,1)) {
	// value to return
	bmd_analysis rVal;
	// create the Continuous BMD modelds
	cBMDModel<LL, PR>  model(likelihood, prior, fixedB, fixedV, isIncreasing);								  
	// Find the maximum a-posteriori and compute the BMD
	
	optimizationResult OptRes = findMAP<LL, PR>(&model,init);
    //DEBUG_OPEN_LOG("bmds.log", file);
    //DEBUG_LOG(file, "After findMap, optres= " << OptRes.result << ", MAP= " << OptRes.functionV << ", max_parms=\n" << OptRes.max_parms << "\n");

  double BMD = model.returnBMD(BMDType, BMRF, tail_prob); 

  Eigen::MatrixXd result;
	std::vector<double> x;
	std::vector<double> y;	
	if (!std::isinf(BMD) && !std::isnan(BMD)){
		int i = 0; 	
		do{
			result = profile_cBMDNC<LL, PR>(  &model,
							  BMDType,
							  BMD,BMRF,tail_prob,
							  step_size,
							  (gsl_cdf_chisq_Pinv(1.0 - 2 * alpha, 1) + 0.1) / 2.0,  // the plus 0.1 is to go slightly beyond
							  isIncreasing);
			// if we have too few rows we decrease
			// the step size 
			if (result.rows() <= 5)
				step_size *= 0.5; 
			i++; 	
		}while(result.rows() <= 5 &&  !(i > 4));
	
	// Prepare the results to form an X/Y tuple
    	// X - BMD values 
    	// Y - Cumulative Probabilities associated with the corresponding X row
	//cout << result << endl; 
	result = convertresult_to_probs(result);	
	x.resize(result.rows());
	y.resize(result.rows());
	}	
	// compute the CDF for the BMD posterior approximation
    	if (!std::isinf(BMD) && !isnan(BMD) && BMD > 0  // flag numerical thins so it doesn't blow up. 
		&& result.rows() > 5 ) {
		for (int i = 0; i < x.size(); i++) { x[i] = result(i, 0); y[i] = result(i, 1); }
		bmd_cdf cdf(x, y);
		rVal.BMD_CDF = cdf;
	}
	
	Eigen::MatrixXd  estimated = model.log_likelihood.mean(OptRes.max_parms);

	rVal.expected.resize(estimated.rows());

	for (int i = 0; i < rVal.expected.size(); i++) {
		rVal.expected[i] = estimated(i, 0);
	}


	rVal.MAP_BMD = BMD; 
	rVal.BMR = BMRF;
	rVal.isExtra = false;
	rVal.type    = BMDType; 
	rVal.COV = model.varMatrix(OptRes.max_parms);
	rVal.MAP_ESTIMATE = OptRes.max_parms; 
    //cout << "******&&&&& OptRes.functionV= " << OptRes.functionV << endl;
	rVal.MAP = OptRes.functionV; 												   
	
	return rVal; 
									
}

/**********************************************************************
 * function bmd_continuous_optimization:
 * 		LL        -    likelihood previously defined
 *      PR        -    Prior
 * 	    fixedB    -    boolean vector of which parameters are fixed
 * 	  	fixedV    -    vector of what value parameters are fixed too. 
 *      isIncreasing - true if the BMD should be calculated for an increasing data set
 *  returns: 
 *      Eigen::MatrixXd rVal <- the value that maximizes the model with the data. 
 * *******************************************************************/
template <class LL, class PR>
Eigen::MatrixXd bmd_continuous_optimization(Eigen::MatrixXd Y, Eigen::MatrixXd X, 
                                            Eigen::MatrixXd prior, 
                                            std::vector<bool> fixedB,std::vector<double> fixedV,
                                            bool is_const_var,
                                            bool is_increasing) {
  // value to return
  bool suff_stat = (Y.cols() == 3); // it is a SS model if there are three parameters
  LL      likelihood(Y, X, suff_stat, is_const_var, is_increasing);
  PR   	  model_prior(prior);
  Eigen::MatrixXd rVal;
  // create the Continuous BMD model
  cBMDModel<LL, PR>  model(likelihood, model_prior, fixedB, fixedV, is_increasing);								  
  // Find the maximum a-posteriori and compute the BMD
  optimizationResult OptRes = findMAP<LL, PR>(&model);
  
  rVal = OptRes.max_parms;
  return rVal; 
  
}

/**********************************************************************
 *  function: bmd_analysis_DNC(Eigen::MatrixXd Y, Eigen::MatrixXd D, Eigen::MatrixXd prior,
 *							 double BMR, bool isExtra, double alpha, double step_size)
 * 
 * *******************************************************************/
template <class LL, class PR>
bmd_analysis bmd_analysis_DNC(Eigen::MatrixXd Y, Eigen::MatrixXd D, Eigen::MatrixXd prior,
                              std::vector<bool> fixedB, std::vector<double> fixedV, int degree,
                              double BMR, bool isExtra, double alpha, double step_size) {

	LL      dichotimousM(Y, D, degree);
	PR   	  model_prior(prior);

	
	dBMDModel<LL, PR> model(dichotimousM, model_prior, fixedB, fixedV);
	optimizationResult oR = findMAP<LL, PR>(&model);
	bmd_analysis rVal;
	double BMD = isExtra ? model.extra_riskBMDNC(BMR) : model.added_riskBMDNC(BMR);

  
	Eigen::MatrixXd result; 
	std::vector<double> x;
	std::vector<double> y;	
	if (!std::isinf(BMD) && !std::isnan(BMD)){	
		int i = 0; 
		do{
			
			result = profile_BMDNC<LL, PR>(&model, isExtra,
		   			                  BMD, BMR,
					                  step_size, (gsl_cdf_chisq_Pinv(1.0 - 2 * alpha, 1) + 0.1) / 2.0, true);
		 	// cerr << result << endl << flush;
			if (result.rows() <= 5)
				  step_size *= 0.5; 
			i++; 	
		}while(result.rows() <= 5 &&  !(i > 4));
		
	
		// Prepare the results to form an X/Y tuple
	  // X - BMD values 
	  // Y - Cumulative Probabilities associated with the corresponding X row
		result = convertresult_to_probs(result);	
		x.clear();
		y.clear();
		// fix the cdf so things don't implode
		for (int i = 0; i < result.rows(); i++) { 
		  if (!isnan(result(i, 0)) && !isinf(result(i, 0))){
		    y.push_back(result(i, 1)); 
		    x.push_back(result(i, 0));
		  } 
		} 
		// fix numerical quantile issues
		// so gsl doesn't go BOOM
		for (int i = 1; i < x.size(); i++){
		  if (x[i] <= x[i-1]){
		    for (int kk = i; kk <  x.size(); kk++){
		      x[kk] = x[kk-1] + 1e-6; 
		    }
		  } 
		}
	  	
	}	
	
	if (!std::isinf(BMD) && !isnan(BMD) && BMD > 0  // flag numerical thins so it doesn't blow up. 
	    && result.rows() > 5 ){
    
    
    
		bmd_cdf cdf(x, y);
		rVal.BMD_CDF = cdf;
	}

	Eigen::MatrixXd  estimated_p = model.log_likelihood.mean(oR.max_parms);
	rVal.expected.resize(estimated_p.rows());
	for (int i = 0; i < rVal.expected.size(); i++) {
		rVal.expected[i] = estimated_p(i, 0)*Y(i, 1); 
	}

	rVal.MAP_BMD = BMD; 
	rVal.BMR = BMR;
	rVal.isExtra = isExtra;
	rVal.COV = model.varMatrix(oR.max_parms);
	rVal.MAP_ESTIMATE = oR.max_parms; 
	rVal.MAP = oR.functionV; 
	
	return rVal; 
}


template  <class PR> 
void  RescaleContinuousModel(cont_model CM, Eigen::MatrixXd *prior, Eigen::MatrixXd *betas, 
                             double max_dose, double divisor, 
                             bool is_increasing, bool is_logNormal, bool is_const_var){
  
  PR   	  model_prior(*prior);
  Eigen::MatrixXd  temp =  rescale_parms(*betas, CM, max_dose, divisor, is_logNormal); 
  //fixme: in the future we might need to change a few things
  // if there are more complicated priors
  int adverseR = 0; 
  switch(CM){
    case cont_model::hill:
      model_prior.scale_prior(divisor,0);
      model_prior.scale_prior(divisor,1);
      model_prior.scale_prior(max_dose,2);
      if (!is_logNormal){
         if (is_const_var){
            model_prior.add_mean_prior(2.0*log(divisor),4);
         }else{
            model_prior.add_mean_prior(2.0*log(divisor),5);
         }
      }
      
      break; 
    case cont_model::exp_3:
      adverseR = is_increasing? NORMAL_EXP3_UP:  NORMAL_EXP3_DOWN; 
      model_prior.scale_prior(divisor,0);
      model_prior.scale_prior(1/max_dose,1);
      if (!is_logNormal){
        if (is_const_var){
          model_prior.add_mean_prior(2.0*log(divisor),4);
        }else{
          model_prior.add_mean_prior(2.0*log(divisor),5);
        }
      }
      break; 
    case cont_model::exp_5:
      adverseR = is_increasing? NORMAL_EXP5_UP:  NORMAL_EXP5_DOWN; 
      model_prior.scale_prior(divisor,0);
      model_prior.scale_prior(1/max_dose,1);
      if (!is_logNormal){
          if (is_const_var){
            model_prior.add_mean_prior(2.0*log(divisor),4);
          }else{
            model_prior.add_mean_prior(2.0*log(divisor),5);
          }
      }
      break; 
    case cont_model::power:
      model_prior.scale_prior(divisor,0);
      model_prior.scale_prior(1/max_dose,1);
      model_prior.scale_prior( divisor*(1/max_dose),2); 

      if (!is_logNormal){
        if (is_const_var){
          model_prior.add_mean_prior(2.0*log(divisor),3);
        }else{
          model_prior.add_mean_prior(2.0*log(divisor),4);
        }
      }
      break;
    case cont_model::polynomial:
    default:
      break; 
  }
  
  Eigen::MatrixXd temp2 = model_prior.get_prior(); 
  *prior = temp2; 
  *betas = temp; 
  return; 
  
}


#endif
