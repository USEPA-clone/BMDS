/*
 * continuous_entry_code.cpp
 * 
 * Copyright 2020  NIEHS <matt.wheeler@nih.gov>
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 * 
 * 
 */


 /* 
// a column order matrix px5 where p is 
// the number of parametersd
*/

#include "continuous_entry_code.h"
#include "mcmc_analysis.h"
#include <algorithm>
#include <vector>
#include <limits>
#include <algorithm> 

using namespace std; 
bool convertSStat(Eigen::MatrixXd Y, Eigen::MatrixXd X,
                  Eigen::MatrixXd *SSTAT, Eigen::MatrixXd *SSTAT_LN,
                  Eigen::MatrixXd *UX){
  bool convert = true; 
  
  if (Y.cols() == 1 ){
    // check to see if it can be converted into sufficient statistics4
    int temp = 0; 
    for (int i = 0; i < X.rows(); i++){
      for (int j = 0 ; j < X.rows(); j++){
        if (X(i,0) == X(j,0)){
          temp++; 
        }
      }
      if( temp == 1){
        convert = false; 
      }
    }
    
    if (convert){
      // we can convert the data
       *SSTAT    = createSuffStat( Y, X, false);
       *SSTAT_LN = createSuffStat(Y,X,true); 
        std::vector<double> uniqueX = unique_list(X );
        Eigen::MatrixXd temp_X(uniqueX.size(),1);
        for (int i = 0; i < uniqueX.size(); i++){
          temp_X(i,0) = uniqueX[i]; 
        }
        *UX = temp_X; 
        return true; 
 
     }
  
  }
  
  
    
  return false; 
}
void removeRow(Eigen::MatrixXd& matrix, unsigned int rowToRemove)
{
  unsigned int numRows = matrix.rows()-1;
  unsigned int numCols = matrix.cols();
  
  if( rowToRemove < numRows )
    matrix.block(rowToRemove,0,numRows-rowToRemove,numCols) = matrix.block(rowToRemove+1,0,numRows-rowToRemove,numCols);
  
  matrix.conservativeResize(numRows,numCols);
}

void removeCol(Eigen::MatrixXd& matrix, unsigned int colToRemove)
{
  unsigned int numRows = matrix.rows();
  unsigned int numCols = matrix.cols()-1;
  
  if( colToRemove < numCols )
    matrix.block(0,colToRemove,numRows,numCols-colToRemove) = matrix.block(0,colToRemove+1,numRows,numCols-colToRemove);
  
  matrix.conservativeResize(numRows,numCols);
}


bmd_analysis laplace_logNormal(Eigen::MatrixXd Y,Eigen::MatrixXd X,
                               Eigen::MatrixXd prior, contbmd riskType, cont_model CM,
                               bool is_increasing, 
                               double bmrf,   double bk_prob, 
                               double alpha, double step_size,
                               Eigen::MatrixXd init) {
  
  bool suff_stat = Y.cols() == 1? false:true; 
  
  std::vector<bool> fixedB(prior.rows());
  std::vector<double> fixedV(prior.rows());
  for (int i = 0; i < prior.rows(); i++) {
    fixedB[i] = false;
    fixedV[i] = 0.0;
  }
  
  
  IDcontinuousPrior model_prior(prior);
  
  lognormalEXPONENTIAL_BMD_NC likelihood_lnexp5U(Y, X, suff_stat,  NORMAL_EXP5_UP);
  lognormalEXPONENTIAL_BMD_NC likelihood_lnexp3U(Y, X, suff_stat,  NORMAL_EXP3_UP);
  lognormalEXPONENTIAL_BMD_NC likelihood_lnexp5D(Y, X, suff_stat,  NORMAL_EXP5_DOWN);
  lognormalEXPONENTIAL_BMD_NC likelihood_lnexp3D(Y, X, suff_stat,  NORMAL_EXP3_DOWN);
  lognormalHILL_BMD_NC likelihood_lnhill(Y, X, suff_stat,  0);
  bmd_analysis a;
  switch (CM)
  {
  case cont_model::hill:
    cout << "Running Exponential 4 Model Log-Normality Assumption." << endl;
    a = bmd_analysis_CNC<lognormalHILL_BMD_NC, IDcontinuousPrior>
                            (likelihood_lnhill,  model_prior, fixedB, fixedV,
                              riskType, bmrf, bk_prob,
                              is_increasing, alpha, step_size,init);
    break; 
  case cont_model::exp_3:
    cout << "Running Exponential 3 Model Log-Normality Assumption." << endl;
    if (is_increasing){
      a =  bmd_analysis_CNC<lognormalEXPONENTIAL_BMD_NC, IDcontinuousPrior>
                                    (likelihood_lnexp3U,  model_prior, fixedB, fixedV,
                                     riskType, bmrf,bk_prob,
                                     is_increasing, alpha, step_size,init);
      
    }else{
      a = bmd_analysis_CNC<lognormalEXPONENTIAL_BMD_NC, IDcontinuousPrior>
                                    (likelihood_lnexp3D,  model_prior, fixedB, fixedV,
                                     riskType, bmrf,bk_prob,
                                     is_increasing, alpha, step_size,init);
    }
    removeRow(a.COV,2);
    removeCol(a.COV,2);
    break; 
  case cont_model::exp_5:
  default: 
    cout << "Running Exponential 5 Model Log-Normality Assumption." << endl;
    if (is_increasing){
      a = bmd_analysis_CNC<lognormalEXPONENTIAL_BMD_NC, IDcontinuousPrior>
                                 (likelihood_lnexp5U,  model_prior, fixedB, fixedV,
                                  riskType, bmrf,bk_prob,
                                  is_increasing, alpha, step_size,init);
    }else{
      a = bmd_analysis_CNC<lognormalEXPONENTIAL_BMD_NC, IDcontinuousPrior>
                                  (likelihood_lnexp5D,  model_prior, fixedB, fixedV,
                                  riskType, bmrf,bk_prob,
                                  is_increasing, alpha, step_size,init);
    }
  break; 
  
  }
  
  return a; 
}


bmd_analysis laplace_Normal(Eigen::MatrixXd Y,Eigen::MatrixXd X,
                            Eigen::MatrixXd prior, contbmd riskType, cont_model CM,
                            bool is_increasing, bool bConstVar,
                            double bmrf,   double bk_prob, 
                            double alpha, double step_size, Eigen::MatrixXd init) {
  
  bool suff_stat = Y.cols() == 1? false:true; 
  
  std::vector<bool> fixedB(prior.rows());
  std::vector<double> fixedV(prior.rows());
  
  for (int i = 0; i < prior.rows(); i++) {
    fixedB[i] = false;
    fixedV[i] = 0.0;
  }
  
  
  IDcontinuousPrior model_prior(prior);
  normalHILL_BMD_NC  likelihood_nhill(Y, X, suff_stat, bConstVar, 0);
  normalPOWER_BMD_NC likelihood_power(Y, X, suff_stat, bConstVar, 0);
  normalEXPONENTIAL_BMD_NC likelihood_nexp5U(Y, X, suff_stat, bConstVar, NORMAL_EXP5_UP);
  normalEXPONENTIAL_BMD_NC likelihood_nexp3U(Y, X, suff_stat, bConstVar, NORMAL_EXP3_UP);
  normalEXPONENTIAL_BMD_NC likelihood_nexp5D(Y, X, suff_stat, bConstVar, NORMAL_EXP5_DOWN);
  normalEXPONENTIAL_BMD_NC likelihood_nexp3D(Y, X, suff_stat, bConstVar, NORMAL_EXP3_DOWN);

  bmd_analysis a;
  switch (CM)
  {
  case cont_model::power:
    if (bConstVar){
      cout << "Running Power Model Normality Assumption using Laplace." << endl;
    }else{
      cout << "Running Power Model Normality-NCV Assumption using Laplace." << endl;
    } 
    a = bmd_analysis_CNC<normalPOWER_BMD_NC, IDcontinuousPrior>
                          (likelihood_power,  model_prior, fixedB, fixedV,
                           riskType, bmrf, bk_prob,
                           is_increasing, alpha, step_size,init);
    break; 
  case cont_model::hill:
    if (bConstVar){
      cout << "Running Hill Model Normality Assumption using Laplace." << endl;
    }else{
      cout << "Running Hill Model Normality-NCV Assumption using Laplace." << endl;
    }
    a = bmd_analysis_CNC<normalHILL_BMD_NC, IDcontinuousPrior>
                    (likelihood_nhill,  model_prior, fixedB, fixedV,
                     riskType, bmrf, bk_prob,
                     is_increasing, alpha, step_size,init);
    break; 
  case cont_model::exp_3:
    if (bConstVar){
      cout << "Running Exponential 3 Model Normality Assumption using Laplace." << endl;
    }else{
      cout << "Running Exponential 3 Model Normality-NCV Assumption using Laplace." << endl;
    }
    if (is_increasing){
      a =  bmd_analysis_CNC<normalEXPONENTIAL_BMD_NC, IDcontinuousPrior>
                          (likelihood_nexp3U,  model_prior, fixedB, fixedV,
                           riskType, bmrf,bk_prob,
                           is_increasing, alpha, step_size,init);
    }else{
      a = bmd_analysis_CNC<normalEXPONENTIAL_BMD_NC, IDcontinuousPrior>
                            (likelihood_nexp3D,  model_prior, fixedB, fixedV,
                             riskType, bmrf,bk_prob,
                             is_increasing, alpha, step_size,init);
    }
    removeRow(a.COV,2);
    removeCol(a.COV,2);
    break; 
  case cont_model::exp_5:
  default: 
    if (bConstVar){
      cout << "Running Exponential 5 Model Normality Assumption using Laplace." << endl;
    }else{
      cout << "Running Exponential 5 Model Normality-NCV Assumption using Laplace." << endl;
    }
    
    if (is_increasing){
      a = bmd_analysis_CNC<normalEXPONENTIAL_BMD_NC, IDcontinuousPrior>
                                    (likelihood_nexp5U,  model_prior, fixedB, fixedV,
                                     riskType, bmrf,bk_prob,
                                     is_increasing, alpha, step_size,init);
    }else{
      a = bmd_analysis_CNC<normalEXPONENTIAL_BMD_NC, IDcontinuousPrior>
                                    (likelihood_nexp5D,  model_prior, fixedB, fixedV,
                                    riskType, bmrf,bk_prob,
                                    is_increasing, alpha, step_size,init);
    }
    break; 
    
  }
  
  return a; 
}

void transfer_continuous_model(bmd_analysis a, continuous_model_result *model){
	if (model){
	  model->nparms = a.COV.rows(); 
		model->max = a.MAP; 
		for (int i = 0; i< model->dist_numE; i ++){
				double temp = double(i)/double(model->dist_numE); 
				model->bmd_dist[i] = a.BMD_CDF.inv(temp);     // BMD @ probability
				model->bmd_dist[model->dist_numE + i] = temp; // probability 
		}
		for (int i = 0; i < model->nparms; i++){
				model->parms[i] = a.MAP_ESTIMATE(i,0); 
				for (int j = 0; j < model->nparms; j++){
					model->cov[i + j*model->nparms] = a.COV(i,j); 
				}
		}
	}
}


void bmd_range_find(continuousMA_result *res, 
					double *range){
 // assume the minimum BMD for the MA is always 0
 range[0] = 0.0; 
 double current_max = 0.0; 
 for (int j = 10; j > 1; j--){
	 for (int i = 0; i < res->nmodels;i++){
		int temp_idx = res->models[i]->dist_numE -j; 
		
		// make sure we are not dealing with an infinite value
		// or not a number
		if (!isnan(res->models[i]->bmd_dist[temp_idx]) && 
			!isinf(res->models[i]->bmd_dist[temp_idx])){
			if ( res->models[i]->bmd_dist[temp_idx] > current_max){
				current_max = res->models[i]->bmd_dist[temp_idx]; 
			}
		}
		 
	 }
 }
 // if we don't find a max then the upper limit is NAN
 range[1] = current_max == 0.0 ? std::numeric_limits<double>::quiet_NaN():current_max; 
  
}

void estimate_ma_laplace(continuousMA_analysis *MA,
                         continuous_analysis *CA ,
                         continuousMA_result *res){
  // standardize the data
  int n_rows = CA->n; int n_cols = CA->suff_stat?3:1; 
  //cerr << "Sufficient Stat: " << n_cols << endl; 
  Eigen::MatrixXd Y(n_rows,n_cols); 
  Eigen::MatrixXd X(n_rows,1); 
  // copy the origional data
  for (int i = 0; i < n_rows; i++){
    Y(i,0) = CA->Y[i]; 
    X(i,0) = CA->doses[i]; 
    if(CA->suff_stat){
      
      Y(i,1) = CA->sd[i]; 
      Y(i,2) = CA->n_group[i]; 
    }
  }
  
  double divisor = get_diviosor( Y,  X); 
  double  max_dose = X.maxCoeff(); 
  
  Eigen::MatrixXd orig_Y = Y, orig_Y_LN = Y; 
  Eigen::MatrixXd orig_X = X; 
  
  Eigen::MatrixXd SSTAT, SSTAT_LN, UX; 
  Eigen::MatrixXd Y_LN, Y_N;
  
  if(!CA->suff_stat){
    //convert to sufficient statistics for speed if we can
    CA->suff_stat = convertSStat(Y, X, &SSTAT, &SSTAT_LN,&UX); 
    if (CA->suff_stat)// it can be converted
    {
      X = UX; 
      Y_N = cleanSuffStat(SSTAT,UX,false);  
      Y_LN = cleanSuffStat(SSTAT_LN,UX,true); 
      orig_X = UX;  
      orig_Y = SSTAT; 
      orig_Y_LN = SSTAT_LN;
      
    }else{
      Y = (1/divisor)*Y; // scale the data with the divisor term. 
      X = X/max_dose;
      Y_N = Y; 
      Y_LN = Y; 
    }
  }else{
    orig_Y = cleanSuffStat(Y,X,false,false); 
    orig_Y_LN = cleanSuffStat(Y,X,true,false);
    SSTAT = cleanSuffStat(Y,X,false); 
    SSTAT_LN = cleanSuffStat(Y,X,true);
    
    std::vector<double> tux = unique_list(X); 
    UX = Eigen::MatrixXd(tux.size(),1); 
    for (unsigned int i = 0; i < tux.size(); i++){
      UX(i,0) = tux[i]; 
    }
    Y_N = SSTAT; 
    X = UX; 
    Y_LN = SSTAT_LN; 
  }
  
  
  
  if (CA->suff_stat){
    X = UX; 

    Eigen::MatrixXd temp; 
    temp = Y_N.col(2);
    Y_N.col(2) = Y_N.col(1);
    Y_N.col(1) = temp; 
    temp = Y_LN.col(2);
    Y_LN.col(2) = Y_LN.col(1);
    Y_LN.col(1) = temp; 
    temp = orig_Y.col(2);
    orig_Y.col(2) = orig_Y.col(1);
    orig_Y.col(1) = temp; 
    temp = orig_Y_LN.col(2);
    orig_Y_LN.col(2) = orig_Y_LN.col(1);
    orig_Y_LN.col(1) = temp; 
    X = X/max_dose;
  } 
  
  bmd_analysis b[MA->nmodels];
 
#pragma omp parallel
{
#pragma omp for  
  for (int i = 0; i < MA->nmodels; i++ ){
      std::vector<bool> fixedB; 
      std::vector<double> fixedV;
      // on each iteration make sure there parameters are emptied
      fixedB.clear();
      fixedV.clear(); 
      Eigen::MatrixXd tprior(MA->nparms[i],MA->prior_cols[i]);
      for (int m = 0; m < MA->nparms[i]; m++){
        fixedB.push_back(false);
        fixedV.push_back(0.0); 
        for (int n = 0; n < MA->prior_cols[i]; n++){
          tprior(m,n) = MA->priors[i][m + n*MA->nparms[i]]; 
        }
      }
  
      Eigen::MatrixXd init_opt; 
      switch((cont_model)MA->models[i]){
      case cont_model::hill:
          init_opt = MA->disttype[i] == distribution::log_normal ?
          bmd_continuous_optimization<lognormalHILL_BMD_NC,IDPrior> (Y_LN, X, tprior, fixedB, fixedV,
                                                                     MA->disttype[i] != distribution::normal_ncv, CA->isIncreasing):
          bmd_continuous_optimization<normalHILL_BMD_NC,IDPrior>    (Y_N, X, tprior,  fixedB, fixedV, 
                                                                     MA->disttype[i] != distribution::normal_ncv, CA->isIncreasing);
        
          RescaleContinuousModel<IDPrior>((cont_model)MA->models[i], &tprior, &init_opt, 
                                          max_dose, divisor, CA->isIncreasing, MA->disttype[i] == distribution::log_normal,
                                          MA->disttype[i] != distribution::normal_ncv); 
          
        break; 
      case cont_model::exp_3:
      case cont_model::exp_5:
          init_opt = MA->disttype[i] == distribution::log_normal ?
          bmd_continuous_optimization<lognormalEXPONENTIAL_BMD_NC,IDPrior> (Y_LN, X, tprior, fixedB, fixedV,
                                                                            MA->disttype[i] != distribution::normal_ncv, CA->isIncreasing):
          bmd_continuous_optimization<normalEXPONENTIAL_BMD_NC,IDPrior>    (Y_N, X, tprior,  fixedB, fixedV, 
                                                                            MA->disttype[i] != distribution::normal_ncv, CA->isIncreasing);

          RescaleContinuousModel<IDPrior>((cont_model)MA->models[i], &tprior, &init_opt, 
                                          max_dose, divisor, CA->isIncreasing,MA->disttype[i] == distribution::log_normal,
                                          MA->disttype[i] != distribution::normal_ncv); 
          
        break; 
      case cont_model::power: 
          init_opt = MA->disttype[i] == distribution::log_normal ?
          bmd_continuous_optimization<lognormalPOWER_BMD_NC,IDPrior> (Y_LN, X, tprior, fixedB, fixedV,
                                                                      MA->disttype[i] != distribution::normal_ncv, CA->isIncreasing):
          bmd_continuous_optimization<normalPOWER_BMD_NC,IDPrior>    (Y_N, X, tprior,  fixedB, fixedV, 
                                                                      MA->disttype[i] != distribution::normal_ncv, CA->isIncreasing);
          
          RescaleContinuousModel<IDPrior>((cont_model)MA->models[i], &tprior, &init_opt, 
                                          max_dose, divisor, CA->isIncreasing,MA->disttype[i] == distribution::log_normal,
                                          MA->disttype[i] != distribution::normal_ncv); 
          
          break; 
      case cont_model::polynomial:
      default:
          break; 
        
      }
 
      // now you fit it based upon the origional data
      if (MA->disttype[i] == distribution::log_normal){
        
        if (CA->suff_stat ){
          b[i] = laplace_logNormal(orig_Y_LN, orig_X,
                                   tprior, CA->BMD_type, (cont_model)MA->models[i],
                                   CA->isIncreasing, CA->BMR, 
                                   CA->tail_prob,  
                                   CA->alpha, 0.02,init_opt);
        }else{
          b[i] = laplace_logNormal(orig_Y_LN, orig_X,
                                   tprior, CA->BMD_type, (cont_model)MA->models[i],
                                   CA->isIncreasing, CA->BMR, 
                                   CA->tail_prob,  
                                   CA->alpha, 0.02,init_opt);
          
        }
        
      }else{
        
        bool isNCV = MA->disttype[i] == distribution::normal_ncv? false:true; 
        if (CA->suff_stat ){
          b[i] = laplace_Normal(orig_Y, orig_X,
                                tprior, CA->BMD_type, (cont_model)MA->models[i],
                                CA->isIncreasing,isNCV, CA->BMR, 
                                CA->tail_prob,  
                                CA->alpha, 0.02,init_opt);
        }else{
          b[i] = laplace_Normal(orig_Y, orig_X,
                                tprior, CA->BMD_type, (cont_model)MA->models[i],
                                CA->isIncreasing,isNCV, CA->BMR, 
                                CA->tail_prob,  
                                CA->alpha, 0.02,init_opt);
          
        }
        
      }
      
      
  }
} 
  
  double post_probs[MA->nmodels]; 
  double temp =0.0; 
  double max_prob = -1.0*std::numeric_limits<double>::infinity(); 
  for (int i = 0; i < MA->nmodels; i++){
    temp  = 	b[i].MAP_ESTIMATE.rows()/2 * log(2 * M_PI) - b[i].MAP + 0.5*log(max(0.0,b[i].COV.determinant()));
    max_prob = temp > max_prob? temp:max_prob; 
    post_probs[i] = temp; 
  }
  
  double norm_sum = 0.0; 
  
  for (int i = 0; i < MA->nmodels; i++){
    post_probs[i] = post_probs[i] - max_prob + log(MA->modelPriors[i]); //FIX ME: ADD MODEL PROBS
    norm_sum     += exp(post_probs[i]);
    post_probs[i] = exp(post_probs[i]);
  }
  
  for (int j = 0; j < MA->nmodels; j++){
    post_probs[j] = post_probs[j]/ norm_sum; 
    
    for (double  i = 0.0; i <= 0.99; i += 0.01 ){
      if ( isnan(b[j].BMD_CDF.inv(i))){
        post_probs[j] = 0;    // if the cdf has nan in it then it needs a 0 posterior
      }  
    } 
  }
  
  norm_sum = 0.0; 
  for (int i =0; i < MA->nmodels; i++){
    norm_sum += post_probs[i]; 
  }
  
  
  for (int i =0; i < MA->nmodels; i++){
    post_probs[i] = post_probs[i]/norm_sum; 
    res->post_probs[i] = post_probs[i];
    transfer_continuous_model(b[i],res->models[i]);
    res->models[i]->model = MA->models[i]; 
    res->models[i]->dist  = MA->disttype[i];
  }

  double range[2]; 
  
  // define the BMD distribution ranges
  // also get compute the MA BMD list
  bmd_range_find(res,range);
  double range_bmd = range[1] - range[0]; 
  for (int i = 0; i < res->dist_numE; i ++){
    double cbmd = double(i)/double(res->dist_numE)*range_bmd; 
    double prob = 0.0; 
    
    for (int j = 0; j < MA->nmodels; j++){
      prob += isnan(b[j].BMD_CDF.P(cbmd))?0:b[j].BMD_CDF.P(cbmd)*post_probs[j]; 
    }
    res->bmd_dist[i] = cbmd; 
    res->bmd_dist[i+res->dist_numE]  = prob;
  }
  
  return;  
}


/*############################################################################
 * 
 * 
 * 
 * 
##############################################################################*/
mcmcSamples mcmc_logNormal(Eigen::MatrixXd Y,Eigen::MatrixXd X,
                            Eigen::MatrixXd prior, contbmd riskType, cont_model CM,
                            bool is_increasing, 
                            double bmrf,   double bk_prob, 
                            double alpha,  int samples, int burnin,  
                            Eigen::MatrixXd initV) {
   
  bool suff_stat = Y.cols() == 1? false:true; 
  std::vector<bool> fixedB(prior.rows());
  std::vector<double> fixedV(prior.rows());
  for (int i = 0; i < prior.rows(); i++) {
    fixedB[i] = false;
    fixedV[i] = 0.0;
  }

  mcmcSamples a;
  int adverseR; 
  switch (CM)
  {
  case cont_model::hill:
    cout << "Running Hill Model Log-Normality Assumption using MCMC." << endl;
    
     a =  MCMC_bmd_analysis_CONTINUOUS_LOGNORMAL<lognormalHILL_BMD_NC, IDcontinuousPrior>
                                    (Y,  X, prior, fixedB, fixedV, is_increasing,
                                     bk_prob,suff_stat,bmrf, riskType, alpha,
                                     samples,0,initV); 
    break; 
  case cont_model::exp_3:
      adverseR = is_increasing?NORMAL_EXP3_UP: NORMAL_EXP3_DOWN; 
      cout << "Running Exponential 3 Model Log-Normality Assumption using MCMC." << endl;
      a =  MCMC_bmd_analysis_CONTINUOUS_LOGNORMAL<lognormalEXPONENTIAL_BMD_NC, IDcontinuousPrior>
                                              (Y,  X, prior, fixedB, fixedV, is_increasing,
                                              bk_prob,suff_stat,bmrf, riskType, alpha,
                                              samples,adverseR,initV);
    break; 
  case cont_model::exp_5:
  default: 
      adverseR = is_increasing?NORMAL_EXP5_UP: NORMAL_EXP5_DOWN; 
      cout << "Running Exponential 5 Model Log-Normality Assumption using MCMC." << endl;
      a =  MCMC_bmd_analysis_CONTINUOUS_LOGNORMAL<lognormalEXPONENTIAL_BMD_NC, IDcontinuousPrior>
                                              (Y,  X, prior, fixedB, fixedV, is_increasing,
                                               bk_prob,suff_stat,bmrf, riskType, alpha,
                                               samples,adverseR,initV);
    break; 
    
  }
  
  return a; 
}

mcmcSamples mcmc_Normal(Eigen::MatrixXd Y,Eigen::MatrixXd X,
                         Eigen::MatrixXd prior, contbmd riskType, cont_model CM,
                         bool is_increasing, bool bConstVar,
                         double bmrf,   double bk_prob, 
                         double alpha, int samples,
                         int burnin, Eigen::MatrixXd initV) {
  
  bool suff_stat = Y.cols() == 1? false:true; 
  std::vector<bool> fixedB(prior.rows());
  std::vector<double> fixedV(prior.rows());
  
  for (int i = 0; i < prior.rows(); i++) {
    fixedB[i] = false;
    fixedV[i] = 0.0;
  }
  
  mcmcSamples a;
  int adverseR; 
  switch (CM)
  {
  case cont_model::hill:
    if (bConstVar){
      cout << "Running Hill Model Normality Assumption using MCMC." << endl;
    }else{
      cout << "Running Hill Model Normality-NCV Assumption using MCMC." << endl;
    }
    
      a =  MCMC_bmd_analysis_CONTINUOUS_NORMAL<normalHILL_BMD_NC, IDcontinuousPrior>
                                                (Y,  X, prior, fixedB, fixedV, is_increasing,
                                                 bk_prob,suff_stat,bmrf, riskType,bConstVar, alpha,
                                                 samples,0,initV); 
    break; 
  case cont_model::exp_3:
    adverseR = is_increasing?NORMAL_EXP3_UP: NORMAL_EXP3_DOWN; 
    if (bConstVar){
      cout << "Running Exponential 3 Model Normality Assumption using MCMC." << endl;
    }else{
      cout << "Running Exponential 3 Model Normality-NCV Assumption using MCMC." << endl;
    }
    a =  MCMC_bmd_analysis_CONTINUOUS_NORMAL<normalEXPONENTIAL_BMD_NC, IDcontinuousPrior>
                                                (Y,  X, prior, fixedB, fixedV, is_increasing,
                                                 bk_prob,suff_stat,bmrf, riskType,bConstVar, alpha,
                                                 samples,adverseR,initV);
    break; 
  case cont_model::exp_5:
  
    adverseR = is_increasing?NORMAL_EXP5_UP: NORMAL_EXP5_DOWN; 
    if (bConstVar){
      cout << "Running Exponential 5 Model Normality Assumption using MCMC." << endl;
    }else{
      cout << "Running Exponential 5 Model Normality-NCV Assumption using MCMC." << endl;
    }
    a =  MCMC_bmd_analysis_CONTINUOUS_NORMAL<normalEXPONENTIAL_BMD_NC, IDcontinuousPrior>
                                                (Y,  X, prior, fixedB, fixedV, is_increasing,
                                                 bk_prob,suff_stat,bmrf, riskType,bConstVar, alpha,
                                                 samples,0,initV);
    break; 
  case cont_model::power:
  default:  
    if (bConstVar){
      cout << "Running Power Model Normality Assumption using MCMC." << endl;
    }else{
      cout << "Running Power Model Normality-NCV Assumption using MCMC." << endl;
    }
    a =  MCMC_bmd_analysis_CONTINUOUS_NORMAL<normalPOWER_BMD_NC, IDcontinuousPrior>
                                              (Y,  X, prior, fixedB, fixedV, is_increasing,
                                              bk_prob,suff_stat,bmrf, riskType,bConstVar, alpha,
                                              samples,adverseR,initV);
    
  
  break; 
    
  }
  //convert a stuff
  //
  return a; 
}

bmd_analysis create_bmd_analysis_from_mcmc(unsigned int burnin, mcmcSamples s){
  bmd_analysis rV;
  rV.MAP          = s.map; 
  rV.MAP_ESTIMATE = s.map_estimate; 
  rV.COV          = s.map_cov; 
  

  std::vector<double> v; 
  for (int i = burnin; i < s.BMD.cols(); i++){
    
    if (!isnan(s.BMD(0,i)) && !isinf(s.BMD(0,i)) && s.BMD(0,i) < 1e9){
	        v.push_back(s.BMD(0,i));   // get rid of the burn in samples
    }
  
  }

  std::vector<double>  prob;
  std::vector<double> bmd_q; 
  double inf_prob =  double(v.size())/double(s.BMD.cols()-burnin); 
  if (v.size() > 0){
    std::sort(v.begin(), v.end());
    for (double k = 0.004; k <= 0.9999; k += 0.005){
    	    prob.push_back(k*inf_prob); 
          int idx = int(k*double(v.size()));
          idx = idx == 0? 0: idx-1; 
    	    bmd_q.push_back(v[idx]);
    }
    // fix numerical quantile issues.
    for (int i = 1; i < bmd_q.size(); i++){
      if (bmd_q[i] <= bmd_q[i-1]){
         for (int kk = i; kk <  bmd_q.size(); kk++){
            bmd_q[kk] = bmd_q[kk-1] + 1e-6; 
         }
      } 
 
    }
  
    if (prob.size() > 10 && *min_element(bmd_q.begin(), bmd_q.end())  < 1e10 
                         && bmd_q[0] > 0 ){  
        rV.BMD_CDF = bmd_cdf(prob,bmd_q);
    }
  }
  return rV; 
}

void transfer_mcmc_output(mcmcSamples a, bmd_analysis_MCMC *b){
  if (b){
    b->samples = a.samples.cols(); 
    b->nparms  = a.samples.rows(); 

    for(unsigned int i= 0; i < a.BMD.cols();  i++){
      b->BMDS[i] = a.BMD(0,i); 
      for(unsigned int j = 0; j < a.samples.rows(); j++){
        b->parms[i + j*a.BMD.cols()] = a.samples(j,i); 
      }
    }
  }
}



void estimate_ma_MCMC(continuousMA_analysis *MA,
                      continuous_analysis   *CA,
                      continuousMA_result   *res,
                      ma_MCMCfits           *ma){ 
  // standardize the data
  int n_rows = CA->n; int n_cols = CA->suff_stat?3:1; 
  Eigen::MatrixXd Y(n_rows,n_cols); 
  Eigen::MatrixXd X(n_rows,1); 
  // copy the origional data
  for (int i = 0; i < n_rows; i++){
    Y(i,0) = CA->Y[i]; 
    X(i,0) = CA->doses[i]; 
    if(CA->suff_stat){
      
      Y(i,1) = CA->sd[i]; 
      Y(i,2) = CA->n_group[i]; 
    }
  }
  
  double divisor = get_diviosor( Y,  X); 
  double  max_dose = X.maxCoeff(); 
 
  Eigen::MatrixXd orig_Y = Y, orig_Y_LN = Y; 
  Eigen::MatrixXd orig_X = X; 
  
  Eigen::MatrixXd SSTAT, SSTAT_LN, UX; 
  Eigen::MatrixXd Y_LN, Y_N;
 
  if(!CA->suff_stat){
    //convert to sufficient statistics for speed if we can
    CA->suff_stat = convertSStat(Y, X, &SSTAT, &SSTAT_LN,&UX); 
    if (CA->suff_stat)// it can be converted
    {
      X = UX; 
      Y_N = cleanSuffStat(SSTAT,UX,false);  
      Y_LN = cleanSuffStat(SSTAT_LN,UX,true); 
      orig_X = UX;  
      orig_Y = SSTAT; 
      orig_Y_LN = SSTAT_LN;
      
    }else{
      Y = (1/divisor)*Y; // scale the data with the divisor term. 
      X = X/max_dose;
      Y_N = Y; 
      Y_LN = Y; 
    }
  }else{
    orig_Y = cleanSuffStat(Y,X,false,false); 
    orig_Y_LN = cleanSuffStat(Y,X,true,false);
    SSTAT = cleanSuffStat(Y,X,false); 
    SSTAT_LN = cleanSuffStat(Y,X,true);
    
    std::vector<double> tux = unique_list(X); 
    UX = Eigen::MatrixXd(tux.size(),1); 
    for (unsigned int i = 0; i < tux.size(); i++){
      UX(i,0) = tux[i]; 
    }
    Y_N = SSTAT; 
    X = UX; 
    Y_LN = SSTAT_LN; 
  }
  
 
  
  if (CA->suff_stat){
    X = UX; 
    //  Y_N = cleanSuffStat(SSTAT,UX,false);  
    //  Y_LN = cleanSuffStat(SSTAT_LN,UX,true); 
    Eigen::MatrixXd temp; 
    temp = Y_N.col(2);
    Y_N.col(2) = Y_N.col(1);
    Y_N.col(1) = temp; 
    temp = Y_LN.col(2);
    Y_LN.col(2) = Y_LN.col(1);
    Y_LN.col(1) = temp; 
    temp = orig_Y.col(2);
    orig_Y.col(2) = orig_Y.col(1);
    orig_Y.col(1) = temp; 
    temp = orig_Y_LN.col(2);
    orig_Y_LN.col(2) = orig_Y_LN.col(1);
    orig_Y_LN.col(1) = temp; 
    X = X/max_dose;
  }

  
  mcmcSamples a[MA->nmodels];

  unsigned int samples = CA->samples; 
  unsigned int burnin  = CA->burnin;  
  
#pragma omp parallel
{
#pragma omp for
  for (int i = 0; i < MA->nmodels; i++ ){
    std::vector<bool> fixedB; 
    std::vector<double> fixedV; 
    fixedB.clear(); // on each iteration make sure there parameters are emptied
    fixedV.clear(); 
    Eigen::MatrixXd tprior(MA->nparms[i],MA->prior_cols[i]);
    for (int m = 0; m < MA->nparms[i]; m++){
        fixedB.push_back(false);
        fixedV.push_back(0.0); 
        for (int n = 0; n < MA->prior_cols[i]; n++){
          tprior(m,n) = MA->priors[i][m + n*MA->nparms[i]]; 
        }
    }

    
   Eigen::MatrixXd init_opt; 
   switch((cont_model)MA->models[i]){
       case cont_model::hill:
          init_opt = MA->disttype[i] == distribution::log_normal ?
                     bmd_continuous_optimization<lognormalHILL_BMD_NC,IDPrior> (Y_LN, X, tprior, fixedB, fixedV,
                                                                                MA->disttype[i] != distribution::normal_ncv, CA->isIncreasing):
                     bmd_continuous_optimization<normalHILL_BMD_NC,IDPrior>    (Y_N, X, tprior,  fixedB, fixedV, 
                                                                                MA->disttype[i] != distribution::normal_ncv, CA->isIncreasing);
          //updated prior updated 
           RescaleContinuousModel<IDPrior>((cont_model)MA->models[i], &tprior, &init_opt, 
                                          max_dose, divisor, CA->isIncreasing, MA->disttype[i] == distribution::log_normal,
                                          MA->disttype[i] != distribution::normal_ncv); 
           
         break; 
       case cont_model::exp_3:
       case cont_model::exp_5:
         init_opt = MA->disttype[i] == distribution::log_normal ?
                   bmd_continuous_optimization<lognormalEXPONENTIAL_BMD_NC,IDPrior> (Y_LN, X, tprior, fixedB, fixedV,
                                                                                     MA->disttype[i] != distribution::normal_ncv, CA->isIncreasing):
                   bmd_continuous_optimization<normalEXPONENTIAL_BMD_NC,IDPrior>    (Y_N, X, tprior,  fixedB, fixedV, 
                                                                                     MA->disttype[i] != distribution::normal_ncv, CA->isIncreasing);
         //updated prior updated 
         //updated prior updated 
         RescaleContinuousModel<IDPrior>((cont_model)MA->models[i], &tprior, &init_opt, 
                                         max_dose, divisor, CA->isIncreasing,MA->disttype[i] == distribution::log_normal,
                                         MA->disttype[i] != distribution::normal_ncv); 
                   
         break; 
       case cont_model::power: 
         init_opt = MA->disttype[i] == distribution::log_normal ?
                   bmd_continuous_optimization<lognormalPOWER_BMD_NC,IDPrior> (Y_LN, X, tprior, fixedB, fixedV,
                                                                               MA->disttype[i] != distribution::normal_ncv, CA->isIncreasing):
                   bmd_continuous_optimization<normalPOWER_BMD_NC,IDPrior>    (Y_N, X, tprior,  fixedB, fixedV, 
                                                                               MA->disttype[i] != distribution::normal_ncv, CA->isIncreasing);
                   
         //updated prior updated 
         RescaleContinuousModel<IDPrior>((cont_model)MA->models[i], &tprior, &init_opt, 
                                                   max_dose, divisor, CA->isIncreasing,MA->disttype[i] == distribution::log_normal,
                                                   MA->disttype[i] != distribution::normal_ncv); 
         
         break; 
       case cont_model::polynomial:
       default:
           break; 
         
     }
      
    a[i] = MA->disttype[i] == distribution::log_normal?
                              mcmc_logNormal(orig_Y_LN, orig_X,
                                                tprior, CA->BMD_type, (cont_model)MA->models[i],
                                                CA->isIncreasing, CA->BMR, 
                                                CA->tail_prob,  
                                                CA->alpha, samples, burnin,init_opt):
                              mcmc_Normal(orig_Y, orig_X,
                                              tprior, CA->BMD_type, (cont_model)MA->models[i],
                                              CA->isIncreasing, MA->disttype[i] != distribution::normal_ncv, CA->BMR,  
                                              CA->tail_prob,  
                                              CA->alpha,samples, burnin,init_opt);
         
      
  }
}  

  bmd_analysis b[MA->nmodels]; 

  for (int i = 0; i < MA->nmodels; i++){
  
    b[i] = create_bmd_analysis_from_mcmc(burnin,a[i]);
  }

  double post_probs[MA->nmodels]; 
  double temp =0.0; 
  double max_prob = -1.0*std::numeric_limits<double>::infinity(); 
  for (int i = 0; i < MA->nmodels; i++){
    temp  = 	b[i].MAP_ESTIMATE.rows()/2 * log(2 * M_PI) - b[i].MAP + 0.5*log(max(0.0,b[i].COV.determinant()));
    max_prob = temp > max_prob? temp:max_prob; 
    post_probs[i] = temp; 
  }
  
  double norm_sum = 0.0; 
 
  for (int i = 0; i < MA->nmodels; i++){
    post_probs[i] = post_probs[i] - max_prob + log(MA->modelPriors[i]); //FIX ME: ADD MODEL PROBS
    norm_sum     += exp(post_probs[i]);
    post_probs[i] = exp(post_probs[i]);
  }

  for (int j = 0; j < MA->nmodels; j++){
    post_probs[j] = post_probs[j]/ norm_sum; 

    for (double  i = 0.0; i <= 0.99; i += 0.01 ){
      if ( isnan(b[j].BMD_CDF.inv(i))){
         post_probs[j] = 0;    // if the cdf has nan in it then it needs a 0 posterior
      }  
    } 
  }
  
  norm_sum = 0.0; 
  for (int i =0; i < MA->nmodels; i++){
    norm_sum += post_probs[i]; 
  }
  

  for (int i =0; i < MA->nmodels; i++){
    post_probs[i] = post_probs[i]/norm_sum; 
    res->post_probs[i] = post_probs[i];
    transfer_continuous_model(b[i],res->models[i]);
    transfer_mcmc_output(a[i],ma->analyses[i]); 
    res->models[i]->model = MA->models[i]; 
    res->models[i]->dist  = MA->disttype[i];
   // cout << i << ":" << b[i].BMD_CDF.inv(0.95) << endl; 
  }
 
  
  double range[2]; 
  
  // define the BMD distribution ranges
  // also get compute the MA BMD list
  bmd_range_find(res,range);
  double range_bmd = range[1] - range[0]; 
  for (int i = 0; i < res->dist_numE; i ++){
    double cbmd = double(i)/double(res->dist_numE)*range_bmd; 
    double prob = 0.0; 
    
    for (int j = 0; j < MA->nmodels; j++){
        prob += isnan(b[j].BMD_CDF.P(cbmd))?0:b[j].BMD_CDF.P(cbmd)*post_probs[j]; 
    }
    res->bmd_dist[i] = cbmd; 
    res->bmd_dist[i+res->dist_numE]  = prob;
  }
  
  return; 
}


/*estimate a single model using laplace/profile likelihood*/
void estimate_sm_laplace(continuous_analysis *CA ,
                         continuous_model_result *res){
  // standardize the data
  int n_rows = CA->n; int n_cols = CA->suff_stat?3:1; 
  //cerr << "Sufficient Stat: " << n_cols << endl; 
  Eigen::MatrixXd Y(n_rows,n_cols); 
  Eigen::MatrixXd X(n_rows,1); 
  // copy the origional data
  for (int i = 0; i < n_rows; i++){
    Y(i,0) = CA->Y[i]; 
    X(i,0) = CA->doses[i]; 
    if(CA->suff_stat){
      
      Y(i,1) = CA->sd[i]; 
      Y(i,2) = CA->n_group[i]; 
    }
  }
  
  double divisor = get_diviosor( Y,  X); 
  double  max_dose = X.maxCoeff(); 
  
  Eigen::MatrixXd orig_Y = Y, orig_Y_LN = Y; 
  Eigen::MatrixXd orig_X = X; 
  
  Eigen::MatrixXd SSTAT, SSTAT_LN, UX; 
  Eigen::MatrixXd Y_LN, Y_N;
  
  if(!CA->suff_stat){
    //convert to sufficient statistics for speed if we can
    CA->suff_stat = convertSStat(Y, X, &SSTAT, &SSTAT_LN,&UX); 
    if (CA->suff_stat)// it can be converted
    {
      X = UX; 
      Y_N = cleanSuffStat(SSTAT,UX,false);  
      Y_LN = cleanSuffStat(SSTAT_LN,UX,true); 
      orig_X = UX;  
      orig_Y = SSTAT; 
      orig_Y_LN = SSTAT_LN;
      
    }else{
      Y = (1/divisor)*Y; // scale the data with the divisor term. 
      X = X/max_dose;
      Y_N = Y; 
      Y_LN = Y; 
    }
  }else{
    orig_Y = cleanSuffStat(Y,X,false,false); 
    orig_Y_LN = cleanSuffStat(Y,X,true,false);
    SSTAT = cleanSuffStat(Y,X,false); 
    SSTAT_LN = cleanSuffStat(Y,X,true);
    
    std::vector<double> tux = unique_list(X); 
    UX = Eigen::MatrixXd(tux.size(),1); 
    for (unsigned int i = 0; i < tux.size(); i++){
      UX(i,0) = tux[i]; 
    }
    Y_N = SSTAT; 
    X = UX; 
    Y_LN = SSTAT_LN; 
  }
  
  
  if (CA->suff_stat){
    X = UX; 
    
    Eigen::MatrixXd temp; 
    temp = Y_N.col(2);
    Y_N.col(2) = Y_N.col(1);
    Y_N.col(1) = temp; 
    temp = Y_LN.col(2);
    Y_LN.col(2) = Y_LN.col(1);
    Y_LN.col(1) = temp; 
    temp = orig_Y.col(2);
    orig_Y.col(2) = orig_Y.col(1);
    orig_Y.col(1) = temp; 
    temp = orig_Y_LN.col(2);
    orig_Y_LN.col(2) = orig_Y_LN.col(1);
    orig_Y_LN.col(1) = temp; 
    X = X/max_dose;
  } 
  
  bmd_analysis b; 
  std::vector<bool> fixedB; 
  std::vector<double> fixedV; 
  
  Eigen::MatrixXd tprior(CA->parms,CA->prior_cols);
  for (int m = 0; m < CA->parms; m++){
    fixedB.push_back(false);
    fixedV.push_back(0.0); 
    for (int n = 0; n < CA->prior_cols; n++){
      tprior(m,n) = CA->prior[m + n*CA->parms]; 
    }
  }
  
  Eigen::MatrixXd init_opt; 
  switch((cont_model)CA->model){
  case cont_model::hill:
    init_opt = CA->disttype == distribution::log_normal ?
    bmd_continuous_optimization<lognormalHILL_BMD_NC,IDPrior> (Y_LN, X, tprior, fixedB, fixedV,
                                                              CA->disttype != distribution::normal_ncv, CA->isIncreasing):
    bmd_continuous_optimization<normalHILL_BMD_NC,IDPrior>    (Y_N, X, tprior,  fixedB, fixedV, 
                                                               CA->disttype != distribution::normal_ncv, CA->isIncreasing);
    
    RescaleContinuousModel<IDPrior>((cont_model)CA->model, &tprior, &init_opt, 
                                    max_dose, divisor, CA->isIncreasing, CA->disttype == distribution::log_normal,
                                    CA->disttype != distribution::normal_ncv); 
    
    break; 
  case cont_model::exp_3:
  case cont_model::exp_5:
    init_opt = CA->disttype == distribution::log_normal ?
    bmd_continuous_optimization<lognormalEXPONENTIAL_BMD_NC,IDPrior> (Y_LN, X, tprior, fixedB, fixedV,
                                                                      CA->disttype != distribution::normal_ncv, CA->isIncreasing):
    bmd_continuous_optimization<normalEXPONENTIAL_BMD_NC,IDPrior>    (Y_N, X, tprior,  fixedB, fixedV, 
                                                                      CA->disttype != distribution::normal_ncv, CA->isIncreasing);
    
    RescaleContinuousModel<IDPrior>((cont_model)CA->model, &tprior, &init_opt, 
                                    max_dose, divisor, CA->isIncreasing, CA->disttype == distribution::log_normal,
                                    CA->disttype != distribution::normal_ncv); 
    
    break; 
  case cont_model::power: 
    init_opt = CA->disttype == distribution::log_normal ?
    bmd_continuous_optimization<lognormalPOWER_BMD_NC,IDPrior> (Y_LN, X, tprior, fixedB, fixedV,
                                                                CA->disttype != distribution::normal_ncv, CA->isIncreasing):
    bmd_continuous_optimization<normalPOWER_BMD_NC,IDPrior>    (Y_N, X, tprior,  fixedB, fixedV, 
                                                                CA->disttype != distribution::normal_ncv, CA->isIncreasing);
    
    RescaleContinuousModel<IDPrior>((cont_model)CA->model, &tprior, &init_opt, 
                                    max_dose, divisor, CA->isIncreasing,CA->disttype == distribution::log_normal,
                                    CA->disttype != distribution::normal_ncv); 
    
    break; 
  case cont_model::polynomial:
  default:
    break; 
    
  }
  
  // now you fit it based upon the origional data
  if (CA->disttype == distribution::log_normal){
    
    if (CA->suff_stat ){
      b = laplace_logNormal(orig_Y_LN, orig_X,
                            tprior, CA->BMD_type, (cont_model)CA->model,
                            CA->isIncreasing, CA->BMR, 
                            CA->tail_prob,  
                            CA->alpha, 0.02,init_opt);
    }else{
      b = laplace_logNormal(orig_Y_LN, orig_X,
                            tprior, CA->BMD_type, (cont_model)CA->model,
                            CA->isIncreasing, CA->BMR, 
                            CA->tail_prob,  
                            CA->alpha, 0.02,init_opt);
      
    }
    
  }else{
    
    bool isNCV = CA->disttype != distribution::normal_ncv; 
    if (CA->suff_stat ){
      b = laplace_Normal(orig_Y, orig_X,
                        tprior, CA->BMD_type, (cont_model) CA->model,
                        CA->isIncreasing,isNCV, CA->BMR, 
                        CA->tail_prob,  
                        CA->alpha, 0.02,init_opt);
    }else{
      b = laplace_Normal(orig_Y, orig_X,
                         tprior, CA->BMD_type, (cont_model)CA->model,
                         CA->isIncreasing,isNCV, CA->BMR, 
                         CA->tail_prob,  
                         CA->alpha, 0.02,init_opt);
    }
  }
 
  transfer_continuous_model(b,res);
  res->model = CA->model; 
  res->dist  = CA->disttype; 
  return;  
}


void estimate_sm_mcmc(continuous_analysis *CA,
                      continuous_model_result *res,
                      bmd_analysis_MCMC *mcmc)
{ 
  // standardize the data
  int n_rows = CA->n; int n_cols = CA->suff_stat?3:1; 
  Eigen::MatrixXd Y(n_rows,n_cols); 
  Eigen::MatrixXd X(n_rows,1); 
  // copy the origional data
  for (int i = 0; i < n_rows; i++){
    Y(i,0) = CA->Y[i]; 
    X(i,0) = CA->doses[i]; 
    if(CA->suff_stat){
      
      Y(i,1) = CA->sd[i]; 
      Y(i,2) = CA->n_group[i]; 
    }
  }
  
  double divisor = get_diviosor( Y,  X); 
  double  max_dose = X.maxCoeff(); 
  
  Eigen::MatrixXd orig_Y = Y, orig_Y_LN = Y; 
  Eigen::MatrixXd orig_X = X; 
  
  Eigen::MatrixXd SSTAT, SSTAT_LN, UX; 
  Eigen::MatrixXd Y_LN, Y_N;
  
  if(!CA->suff_stat){
    //convert to sufficient statistics for speed if we can
    CA->suff_stat = convertSStat(Y, X, &SSTAT, &SSTAT_LN,&UX); 
    if (CA->suff_stat)// it can be converted
    {
      X = UX; 
      Y_N = cleanSuffStat(SSTAT,UX,false);  
      Y_LN = cleanSuffStat(SSTAT_LN,UX,true); 
      orig_X = UX;  
      orig_Y = SSTAT; 
      orig_Y_LN = SSTAT_LN;
      
    }else{
      Y = (1/divisor)*Y; // scale the data with the divisor term. 
      X = X/max_dose;
      Y_N = Y; 
      Y_LN = Y; 
    }
  }else{
    orig_Y = cleanSuffStat(Y,X,false,false); 
    orig_Y_LN = cleanSuffStat(Y,X,true,false);
    SSTAT = cleanSuffStat(Y,X,false); 
    SSTAT_LN = cleanSuffStat(Y,X,true);
    
    std::vector<double> tux = unique_list(X); 
    UX = Eigen::MatrixXd(tux.size(),1); 
    for (unsigned int i = 0; i < tux.size(); i++){
      UX(i,0) = tux[i]; 
    }
    Y_N = SSTAT; 
    X = UX; 
    Y_LN = SSTAT_LN; 
  }
  
  if (CA->suff_stat){
    X = UX; 
    //  Y_N = cleanSuffStat(SSTAT,UX,false);  
    //  Y_LN = cleanSuffStat(SSTAT_LN,UX,true); 
    Eigen::MatrixXd temp; 
    temp = Y_N.col(2);
    Y_N.col(2) = Y_N.col(1);
    Y_N.col(1) = temp; 
    temp = Y_LN.col(2);
    Y_LN.col(2) = Y_LN.col(1);
    Y_LN.col(1) = temp; 
    temp = orig_Y.col(2);
    orig_Y.col(2) = orig_Y.col(1);
    orig_Y.col(1) = temp; 
    temp = orig_Y_LN.col(2);
    orig_Y_LN.col(2) = orig_Y_LN.col(1);
    orig_Y_LN.col(1) = temp; 
    X = X/max_dose;
  }
  
  
  mcmcSamples a;
  unsigned int samples = CA->samples; 
  unsigned int burnin  = CA->burnin;  
  std::vector<bool> fixedB; 
  std::vector<double> fixedV; 
  
  fixedB.clear(); // on each iteration make sure there parameters are emptied
  fixedV.clear(); 
  Eigen::MatrixXd tprior(CA->parms,CA->prior_cols);
  for (int m = 0; m < CA->parms; m++){
    fixedB.push_back(false);
    fixedV.push_back(0.0); 
    for (int n = 0; n < CA->prior_cols; n++){
        tprior(m,n) = CA->prior[m + n*CA->parms]; 
    }
  }
    
    
  Eigen::MatrixXd init_opt; 
  switch((cont_model)CA->model){
    case cont_model::hill:
      init_opt = CA->disttype == distribution::log_normal ?
      bmd_continuous_optimization<lognormalHILL_BMD_NC,IDPrior> (Y_LN, X, tprior, fixedB, fixedV,
                                                                 CA->disttype != distribution::normal_ncv, CA->isIncreasing):
      bmd_continuous_optimization<normalHILL_BMD_NC,IDPrior>    (Y_N, X, tprior,  fixedB, fixedV, 
                                                                 CA->disttype != distribution::normal_ncv, CA->isIncreasing);
      //updated prior updated 
      RescaleContinuousModel<IDPrior>((cont_model)CA->model, &tprior, &init_opt, 
                                      max_dose, divisor, CA->isIncreasing, CA->disttype == distribution::log_normal,
                                      CA->disttype != distribution::normal_ncv); 
      
      break; 
    case cont_model::exp_3:
    case cont_model::exp_5:
      init_opt = CA->disttype == distribution::log_normal ?
      bmd_continuous_optimization<lognormalEXPONENTIAL_BMD_NC,IDPrior> (Y_LN, X, tprior, fixedB, fixedV,
                                                                        CA->disttype != distribution::normal_ncv, CA->isIncreasing):
      bmd_continuous_optimization<normalEXPONENTIAL_BMD_NC,IDPrior>    (Y_N, X, tprior,  fixedB, fixedV, 
                                                                        CA->disttype != distribution::normal_ncv, CA->isIncreasing);
      //updated prior updated 
      //updated prior updated 
      RescaleContinuousModel<IDPrior>((cont_model)CA->model, &tprior, &init_opt, 
                                      max_dose, divisor, CA->isIncreasing,CA->disttype == distribution::log_normal,
                                      CA->disttype != distribution::normal_ncv); 
      
      break; 
    case cont_model::power: 
      init_opt = CA->disttype == distribution::log_normal ?
      bmd_continuous_optimization<lognormalPOWER_BMD_NC,IDPrior> (Y_LN, X, tprior, fixedB, fixedV,
                                                                  CA->disttype != distribution::normal_ncv, CA->isIncreasing):
      bmd_continuous_optimization<normalPOWER_BMD_NC,IDPrior>    (Y_N, X, tprior,  fixedB, fixedV, 
                                                                  CA->disttype != distribution::normal_ncv, CA->isIncreasing);
      
      //updated prior updated 
      RescaleContinuousModel<IDPrior>((cont_model)CA->model, &tprior, &init_opt, 
                                      max_dose, divisor, CA->isIncreasing,CA->disttype == distribution::log_normal,
                                      CA->disttype != distribution::normal_ncv); 
      
      break; 
    case cont_model::polynomial:
    default:
      break; 
      
    }
    
    a = CA->disttype == distribution::log_normal?
      mcmc_logNormal(orig_Y_LN, orig_X,
                     tprior, CA->BMD_type, (cont_model)CA->model,
                     CA->isIncreasing, CA->BMR, 
                     CA->tail_prob,  
                     CA->alpha, samples, burnin,init_opt):
      mcmc_Normal(orig_Y, orig_X,
                  tprior, CA->BMD_type, (cont_model)CA->model,
                  CA->isIncreasing, CA->disttype != distribution::normal_ncv, CA->BMR,  
                  CA->tail_prob,  
                  CA->alpha,samples, burnin,init_opt);
    
  bmd_analysis b; 
 
  b = create_bmd_analysis_from_mcmc(burnin,a);
  transfer_continuous_model(b,res);
  transfer_mcmc_output(a,mcmc); 
  res->model = CA->model; 
  res->dist  = CA->disttype;
 
  return; 
}



