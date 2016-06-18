#include <vector>
#include <iostream>
#include <ilcplex/ilocplex.h>
#include "arc.h"
#include "t.h"

#include "model_flux.h"

using namespace std;
ILOSTLBEGIN

void modelFlux(int l, int N[],int  ord[], bool extra, vector<vector<Arc>> O, int n, int o){

    IloEnv env;

    try{

        IloModel model(env);

        // var z_{k,mi}
        IloArray<IloBoolVarArray> z(env, l);

        for(int i = 0; i < l; i++){

            z[i] = IloBoolVarArray (env, o);

        }

        // var x_{kiab}
        IloArray<IloArray<IloArray<IloBoolVarArray>>> x(env, l);
        for (int i = 0; i < l; i++) {
            x[i] = IloArray<IloArray<IloBoolVarArray> > (env, n);
            for (int j = 0; j < n; j++) {
                x[i][j] = IloArray<IloBoolVarArray> (env, n);
                for (int k = 0; k < n; k++) {
                    x[i][j][k] = IloBoolVarArray (env,n);
                }
            }
        }

        //objective function

        IloExpr obj(env);

        for(int k = 0; k < l;k++){
            for(int i = 1; i < o; i++){
                obj += z[k][i];
            }
        }

        model.add(IloMinimize(env, obj));

        //one operator by level
        for(int k = 0; k < l; k++){
            model.add(IloSum(z[k]) == 1);
        }

        // outgoing
        for(int i = 0; i < n; i++){
           model.add(IloSum(x[0][i][i]) == 1);
        }

        // ingoing
        for(int i = 0; i < n; i++){
            IloExpr expr(env);
            for(int j = 0; j < n; j++){
                expr += x[l-1][i][j][N[i]];
            }
            model.add(expr == 1);
            expr.end();
        }

        //conservation
        for(int i = 0; i < n; i++){
            for(int k = 1; k < l; k++){
                for(int a = 0; a < n; a++){
                    IloExpr expr(env);

                    for(int j = 0; j < n; j++){
                        expr +=  x[k][i][a][j];
                        expr -=  x[k-1][i][j][a];
                    }

                    model.add(expr == 0);
                    expr.end();
                }

            }
        }

        // just the arcs that are in mi can be used


            for(int a = 0; a < n; a++){
                for(int b = 0; b < n; b++){
                    for(int k  = 0; k < l; k++){
                        IloExpr flowCapacityTerm1(env);
                        IloExpr flowCapacityTerm2(env);

                        for(int i = 0; i < n; i++){
                            flowCapacityTerm1 += x[k][i][a][b];
                        }

                        for(int y = 0; y < o; y++){
                            Arc arc(a,b);
                            if(contains(arc, O[y])){
                                flowCapacityTerm2 += z[k][y];
                            }
                        }

                        model.add(flowCapacityTerm1 <= flowCapacityTerm2);

                        flowCapacityTerm1.end();
                        flowCapacityTerm2.end();
                }
            }
        }


        // constraint to u0 be always in the end
        for(int k = 0; k < l-1; k++){
            model.add(z[k][0] <= z[k+1][0]);
        }

        //extra
        if(extra){
            //cout << "Adicionando restrições extras" << endl;
            for (int k = 0; k < l-1; k++){

                //cout << "k = " << k << endl;
                for (int i = 0; i < (n-1); i++){

                    //cout << "a2 = " << a2 << endl;
                    for(int b = 0; b < (n-1); b++){

                        //cout << "b = " << b << endl;
                            //for(int j = 1; (k+j) <= (l-1); j++){
                                //cout << "b2 = " << b2 << endl;
                                //cout << ord[i] << endl;
                                //cout << ord[i+1] << endl;
                                //int j = (l-1) - k;
                                IloExpr expr1(env);
                                IloExpr expr2(env);
                                for(int a = 0; a < n; a ++){
                                    expr1 += x[k][ord[i]][a][b];
                                    expr2 += x[k][ord[i+1]][a][b+1];
                                }

                                IloExpr expr3(env);
                                IloExpr expr4(env);
                                for(int b2 = 0; b2 < (n-1); b2++){
                                    expr4 = x[k+1][ord[i]][b][b2] - x[k+1][ord[i+1]][b+1][b2+1];
                                }

                                model.add(expr1 + expr2 + expr4 <= 2);

                                //model.add(expr1 + expr2 + x[k+j][ord[i]][b][b2] <= x[k+j][ord[i+1]][b+1][b2+1] + 2);

                                //model.add(expr1 + expr2 + x[k+1][ord[i]][b][b2] <= x[k+1][ord[i+1]][b+1][b2+1] + 2);
                            //}

                    }
                }
            }

            for(int k = 0; k < (l-1); k++){
                IloExpr expr(env);
                for(int a = 0; a < n; a++){
                    //expr += x[k][ord[0]][a][0];
                    model.add(x[k][ord[0]][a][0] <= x[k+1][ord[0]][0][0]);
                }
                //model.add(expr <= x[k+1][ord[0]][0][0]);

            }

            for(int k = 0; k < (l-1); k++){
                IloExpr expr(env);
                for(int a = 0; a < n; a++){
                    //expr += x[k][ord[n-1]][a][n-1];
                    model.add(x[k][ord[n-1]][a][n-1] <= x[k+1][ord[n-1]][n-1][n-1]);
                }
                //model.add(expr <= x[k+1][ord[n-1]][n-1][n-1]);
            }
        }

        //Solving the problem
        //env.setOut(env.getNullStream());
        IloCplex cplex(model);
        //cplex.setOut(env.getNullStream());
        //cplex.setWarning(env.getNullStream());
        //cplex.setError(env.getNullStream());

        cplex.extract(model);

        //timeout
        cplex.setParam(IloCplex::Param::TimeLimit,7200);
        //IloCplex::Param::MIP::Strategy::HeuristicFreq
        cplex.setParam(IloCplex::Param::MIP::Strategy::HeuristicFreq,-1);
        //IloCplex::Param::MIP::Display

        //cplex.setParam(IloCplex::Param::MIP::Display,4);
        //cplex.setParam(IloCplex::RootAlg,6);
        //cplex.setParam(IloCplex::NodeAlg,6);
        cplex.setParam(IloCplex::Param::MIP::Strategy::VariableSelect, 4);
        //cplex.setParam(IloCplex::MIPEmphasis, 1);
        cplex.setParam(IloCplex::Param::MIP::Tolerances::UpperCutoff, l);

        //cplex.exportModel("/home/sandro/model1.lp");
        if (cplex.solve()) {
            cout << "Optimal value: " << cplex.getObjValue() << endl;
            cout << cplex.getTime() << endl;
        }else{
            cout << "timeout" << endl;
        }

        cplex.end();
        model.end();
        obj.end();


    }catch(IloException& e){
        cerr << "Concert exception caught: " << e << endl;
    }catch(...){
        cerr << "Unknow exception caught" << endl;
    }

    env.end();
}
