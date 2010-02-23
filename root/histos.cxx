//creates Histograms from result database.

#include "sqlite3.h"

//#include "interface/exception.hpp"

#include "TH1.h"
#include "TFile.h"

#include <string>
#include <sstream>
#include <iostream>

#include <cmath>

using namespace std;

//these two routines for easier sqlite3 are taken from database/merge.cpp:
void sqlite3_exec(sqlite3 * db, const char * query){
   char * err = 0;
   sqlite3_exec(db, query, 0, 0, &err);
   if(err!=0){
      stringstream ss;
      ss << err;
      cerr << "sqlite3_exec returned error: " << ss.str() << endl;
      sqlite3_free(err);
      throw ss.str();
   }
}

sqlite3_stmt* sqlite3_prepare(sqlite3 * db, const char* sql){
    sqlite3_stmt * statement = 0;
    int ret = sqlite3_prepare(db, sql, -1, &statement, 0);
    if(ret!=0){
        if(statement!=0){
            sqlite3_finalize(statement);
        }
        stringstream error_ss;
        error_ss << "Could not compile SQL statement " << sql;
        throw error_ss.str();
    }
    return statement;
}

sqlite3* sqlite3_open(const string & fname){
    sqlite3 * db=0;
    int res = sqlite3_open(fname.c_str(), &db);
    if(res!=SQLITE_OK){
        stringstream ss;
        ss << "sqlite3_open(" << fname << "): " << sqlite3_errmsg(db);
        throw ss.str();
    }
    return db;
}

/** Create plots for a producer of type "lnQ-minimize" in directory \c dir.
 * 
void make_plots_lnQ_minimize(sqlite3 * db, TDirectory * dir, const string & p_name){
    
}
*/


//new architecture:
// * do_main reads prodinfo table
// * for each producer, the correct make_plots_* command is run
// * the make_plots_* routines create histos that summarize the most important things.




void do_main(const string & fname, TDirectory * cd, const string & tablename){
    cd->cd();
    TH1F* histo = new TH1F("h", "h", 2000, -100., 100.); //owned by cd
    //    histo->SetMaximum(51.0);
    
    sqlite3 * db = sqlite3_open(fname);
    stringstream ss;
    ss << "SELECT nll_sb - nll_b FROM '" << tablename << "';";
    sqlite3_stmt* st = sqlite3_prepare(db, ss.str().c_str());
    int res;
    while(SQLITE_ROW == (res=sqlite3_step(st))){
        double lnq = sqlite3_column_double(st, 0);
        histo->Fill(lnq);
    }
    if(res!=SQLITE_DONE){
        ss.str("");
        ss << "Error in do_main: stepping through results returned " << res;
        throw ss.str();
    }
}

int main(int argc, char** argv){
    if(argc!=3){
        cerr << "Usage: " << argv[0] << " <.db file>  <tablename>" << endl;
        return 1;
    }
    try{
        TFile file("out_SigAndBkg.root", "recreate");
        do_main(argv[1], &file, argv[2]);
        file.Write();
        file.Close();
    }
    catch(std::string & ex){
        cerr << "Exception while processing: " << endl << ex << endl;
        return 1;
    }
}

