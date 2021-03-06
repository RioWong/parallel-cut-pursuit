/*=============================================================================
 * Hugo Raguet 2018
 *===========================================================================*/
#include <cmath>
#include "../include/omp_num_threads.hpp"
#include "../include/pcd_prox_split.hpp"

#define TPL template <typename real_t>
#define PCD_PROX Pcd_prox<real_t>

/* constants of the correct type */
#define ZERO ((real_t) 0.0)
#define ONE ((real_t) 1.0)
#define TENTH ((real_t) 0.1)

using namespace std;

TPL PCD_PROX::Pcd_prox(size_t size) : size(size)
{
    name = "Preconditioned proximal splitting algorithm";
    objective_values = iterate_evolution = nullptr;
    cond_min = 1e-2;
    dif_rcd = 1e-4;
    dif_tol = 1e-5;
    it_max = 1e4;
    verbose = 1e2;
    eps = numeric_limits<real_t>::epsilon();
    X = nullptr;
}

TPL PCD_PROX::~Pcd_prox(){ free(X); }

TPL void PCD_PROX::set_name(const char* name)
{ this->name = name; }

TPL void PCD_PROX::set_monitoring_arrays(real_t* objective_values,
    real_t* iterate_evolution)
{
    this->objective_values = objective_values;
    this->iterate_evolution = iterate_evolution;
}

TPL void PCD_PROX::set_conditioning_param(real_t cond_min, real_t dif_rcd)
{
    this->cond_min = cond_min;
    this->dif_rcd = dif_rcd;
}

TPL void PCD_PROX::set_algo_param(real_t dif_tol, int it_max, int verbose,
    real_t eps)
{
    this->dif_tol = dif_tol;
    this->it_max = it_max;
    this->verbose = verbose;
    this->eps = eps;
}

TPL void PCD_PROX::set_iterate(real_t* X){ this->X = X; }

TPL real_t* PCD_PROX::get_iterate(){ return this->X; }

TPL void PCD_PROX::initialize_iterate()
{
    if (!X){ X = (real_t*) malloc_check(sizeof(real_t)*size); }
    for (size_t i = 0; i < size; i++){ X[i] = ZERO; }
}

TPL void PCD_PROX::preconditioning(bool init)
{ if (init && !X){ initialize_iterate(); } }

TPL int PCD_PROX::precond_proximal_splitting(bool init)
{
    int it = 0;
    real_t dif = (dif_tol > ONE) ? dif_tol : ONE;
    if (dif_rcd > dif){ dif = dif_rcd; }
    int it_verb;

    if (verbose){
        cout << name << ":" << endl;
        it_verb = 0;
    }

    if (verbose){ cout << "Preconditioning... " << flush; }
    preconditioning(init);
    if (verbose){ cout << "done." << endl; }

    if (init && objective_values){ objective_values[0] = compute_objective(); }

    if (dif_tol > ZERO || dif_rcd > ZERO || iterate_evolution){
        last_X = (real_t*) malloc_check(sizeof(real_t)*size);
        for (size_t i = 0; i < size; i++){ last_X[i] = X[i]; }
    }

    while (it < it_max && dif >= dif_tol){

        if (verbose && it_verb == verbose){
            print_progress(it, dif);
            it_verb = 0;
        }

        if (dif < dif_rcd){
            if (verbose){
                print_progress(it, dif);
                cout << "\nReconditioning... " << flush;
            }
            preconditioning();
            dif_rcd *= TENTH;
            if (verbose){ cout << "done." << endl; }
        }

        main_iteration();

        if (dif_tol > ZERO || dif_rcd || iterate_evolution){
            dif = compute_evolution();
            if (iterate_evolution){ iterate_evolution[it] = dif; }
        }

        it++; it_verb++;

        if (objective_values){ objective_values[it] = compute_objective(); }
        
    }
    
    if (verbose){ print_progress(it, dif); cout << endl; }
    
    if (dif_tol > ZERO || dif_rcd > ZERO || iterate_evolution){ free(last_X); }

    return it;
}

TPL void PCD_PROX::print_progress(int it, real_t dif)
{
    cout << "\r" << "iteration " << it << " (max. " << it_max << "); ";
    if (dif_tol > ZERO || dif_rcd > ZERO){
        cout.precision(2);
        cout << scientific << "iterate evolution " << dif <<  " (recond. "
            << dif_rcd << ", tol. " << dif_tol << ")";
    }
    cout << flush;
}

TPL real_t PCD_PROX::compute_evolution()
/* by default, relative evolution in Euclidean norm */
{
    real_t dif = ZERO;
    real_t norm = ZERO;
    #pragma omp parallel for schedule(static) NUM_THREADS(size) \
        reduction(+:dif, norm)
    for (size_t i = 0; i < size; i++){
        real_t d = last_X[i] - X[i];
        dif += d*d;
        norm += X[i]*X[i];
        last_X[i] = X[i];
    }
    return sqrt(norm) > eps ? sqrt(dif/norm) : sqrt(dif)/eps;
}

/**  instantiate for compilation  **/
template class Pcd_prox<double>;
template class Pcd_prox<float>;
