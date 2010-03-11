//creates Histograms from result database.

#include "sqlite3.h"

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

void do_main(const string & fname, TDirectory * cd, const string & producer_name){
    cd->cd();
    TH1F* histo = new TH1F("h", "h", 100, 22.0616, 36.2339); //owned by cd
    sqlite3 * db = sqlite3_open(fname);
    stringstream ss;
    ss << "SELECT " << producer_name << "__nll_sb - " << producer_name << "__nll_b FROM 'products';";
    sqlite3_stmt* st = sqlite3_prepare(db, ss.str().c_str());
    int res;
    int n;
    while(SQLITE_ROW == (res=sqlite3_step(st))){
        double lnq = sqlite3_column_double(st, 0);
        histo->Fill(sqrt(2 * fabs(lnq)));
    }
    if(res!=SQLITE_DONE){
        ss.str("");
        ss << "Error in do_main: stepping through results returned " << res;
        throw ss.str();
    }
}

int main(int argc, char** argv){
    if(argc!=3){
        cerr << "Usage: " << argv[0] << " <.db file>  <producer name>" << endl;
        return 1;
    }
    try{
        TFile file("theta_significance.root", "recreate");
        do_main(argv[1], &file, argv[2]);
        file.Write();
        file.Close();
    }
    catch(std::string & ex){
        cerr << "Exception while processing: " << endl << ex << endl;
        return 1;
    }
}

