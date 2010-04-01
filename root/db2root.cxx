#include "sqlite3.h"

#include "TTree.h"
#include "TFile.h"

#include <string>
#include <sstream>
#include <iostream>
#include <cstdlib>

using namespace std;

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

void sqlite_error(int code, const string & function){
    cerr << "SQL error " << code << " in " << function << ". Exiting." << endl;
    exit(1);
}

//returns a list of column names in the given table
enum type{ t_double, t_int };
map<string, type> get_columns(sqlite3 * db, const string & table, const string & database=""){
    map<string, type> result;
    stringstream ss;
    if(database!="")
        ss << "PRAGMA " << database << ".";
    else
        ss << "PRAGMA ";
    ss << "table_info('" << table << "');";
    sqlite3_stmt * statement = sqlite3_prepare(db, ss.str().c_str());
    int ret;  
    while((ret=sqlite3_step(statement))==SQLITE_ROW){
        string name = (const char*)sqlite3_column_text(statement, 1);
        string type_name = (const char*)sqlite3_column_text(statement, 2);
        type t;
        if(type_name.find("INTEGER")!=string::npos){
            t = t_int;
        }
        else if(type_name.find("DOUBLE")!=string::npos){
            t = t_double;
        }
        else{
            cout << "warning: not converting column " << table << "." << name << " of unsupported type " << type_name << endl;
            continue;
        }
        result[name] = t;
    }
    sqlite3_finalize(statement);
    if(ret!=SQLITE_DONE){
        sqlite_error(ret, __FUNCTION__); //exits
    }    
    return result;
}

//returns a list of sorted table names in the given database
vector<string> get_tables(sqlite3 * db, const string & database = ""){
    vector<string> result;
    string master_table = "sqlite_master";
    if(database!="") master_table = database + ".sqlite_master";
    stringstream ss;
    ss << "SELECT name FROM " << master_table << " WHERE type='table' ORDER BY name;";
    sqlite3_stmt * statement = sqlite3_prepare(db, ss.str().c_str());
    int ret;  
    while((ret=sqlite3_step(statement))==SQLITE_ROW){
        result.push_back((const char*)sqlite3_column_text(statement, 0));
    }
    sqlite3_finalize(statement);
    if(ret!=SQLITE_DONE){
        sqlite_error(ret, __FUNCTION__); //exits
    }
    return result;
}

union doubleint{
    double d;
    int64_t i;
};

int main(int argc, char ** argv){
    if(argc!=3){
        cerr << "Usage: " << argv[0] << " <db file> <root file>" << endl;
        return 1;
    }
    sqlite3 * db=0;
    int ret;
    if((ret=sqlite3_open(argv[1], &db))!=SQLITE_OK){
        cerr << "Could not open database file " << argv[1] << ": " << sqlite3_errmsg(db) << endl;
        return 1;
    }
    TFile * file = new TFile(argv[2], "recreate");
    if(file->IsZombie()){
        cerr << "Could not open root file " << argv[2] << " for writing." << endl;
        return 1;
    }
    
    vector<string> tables = get_tables(db);
    for(vector<string>::const_iterator it=tables.begin(); it!=tables.end(); ++it){
        map<string, type> columns = get_columns(db, *it);
        doubleint * data = new doubleint[columns.size()];
        TTree * tree = new TTree(it->c_str(), it->c_str());
        
        stringstream ttree_spec;
        stringstream sql_query;
        sql_query << "SELECT ";
        
        for(map<string, type>::const_iterator cit=columns.begin(); cit!=columns.end(); ++cit){
            if(cit!=columns.begin()){
                sql_query << ", ";
                ttree_spec << ":";
            }
            sql_query << cit->first;
            ttree_spec << cit->first << "/";
            switch(cit->second){
                case t_double: ttree_spec << "D"; break;
                case t_int: ttree_spec << "L"; break;
            }
        }
        
        tree->Branch((*it).c_str(), data, ttree_spec.str().c_str());
        
        sql_query << " FROM " << *it << ";";
        sqlite3_stmt* st = sqlite3_prepare(db, sql_query.str().c_str());
        int res;
        while(SQLITE_ROW == (res=sqlite3_step(st))){
            int i=0;
            for(map<string, type>::const_iterator cit=columns.begin(); cit!=columns.end(); ++cit, ++i){
                switch(cit->second){
                    case t_double: data[i].d = sqlite3_column_double(st, i); break;
                    case t_int: data[i].i = sqlite3_column_int(st, i); break;
                }
            }
            tree->Fill();
        }
        sqlite3_finalize(st);//ignore error
        tree->Write();
        delete[] data;
    }
    sqlite3_close(db);//ignore error
    file->Close();
}

