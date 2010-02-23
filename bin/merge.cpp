#include "sqlite3.h"

#include <string>
#include <sstream>
#include <iostream>
#include <stdlib.h>

#include "interface/exception.hpp"

#include <boost/program_options.hpp>
#include <boost/filesystem.hpp>
#include <boost/regex.hpp>

using namespace std;
using namespace theta;

namespace po = boost::program_options;
namespace fs = boost::filesystem;

/** TODO: rewrite such that:
 *  - only compatible runs (=with same prodinfo entries) are merged
 *  - adjust runid, if necessary
 *  - merging is faster for many files
 */

void sqlite3_exec(sqlite3 * db, const char * query){
   char * err = 0;
   sqlite3_exec(db, query, 0, 0, &err);
   if(err!=0){
      stringstream ss;
      ss << err;
      cerr << "sqlite3_exec returned error: " << ss.str() << endl;
      sqlite3_free(err);
      throw Exception(ss.str());
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
        throw Exception(error_ss.str());
    }
    return statement;
}


//merge the sqlite databases in file1 and file2 into file1.
//The files must contain the same tables with the same schema.
//Otherwise, the command will fail with an exception.
//Note that the contents of file1is undefined if trying to merge with a file of different database schema:
// some tables might already be merged, others not.
//TODO: could be done better (=faster), if merging more than two
// files at once ...
void merge(const string & file1, const string & file2){
    sqlite3 * db=0;
    sqlite3_open(file1.c_str(), &db);
    //this saves some time ...:
    sqlite3_exec(db, "PRAGMA journal_mode=OFF;");
    sqlite3_exec(db, "PRAGMA cache_size=5000;");

    stringstream ss;
    ss << "ATTACH \"" << file2 << "\" AS o";
    sqlite3_exec(db, ss.str().c_str());

    //get tables from file1:
    vector<string> tables;
    ss.str("");
    ss << "SELECT name FROM sqlite_master WHERE type='table' ORDER BY name;";
    sqlite3_stmt * statement = sqlite3_prepare(db, ss.str().c_str());
    int ret;
    while((ret=sqlite3_step(statement))==SQLITE_ROW){
        tables.push_back((const char*)sqlite3_column_text(statement, 0));
    }
    sqlite3_finalize(statement);
    if(ret!=SQLITE_DONE){
        cout << "error: " << ret << endl;
        throw Exception("");
    }
    //compare with tables from file2:
    ss.str("");
    ss << "SELECT name FROM o.sqlite_master WHERE type='table' ORDER BY name;";
    statement = sqlite3_prepare(db, ss.str().c_str());
    int i=0;
    while((ret=sqlite3_step(statement))==SQLITE_ROW){
        string tablename((const char*)sqlite3_column_text(statement, 0));
        if(tablename!=tables[i]){
            sqlite3_finalize(statement);
            stringstream error_ss;
            error_ss << "merge: table '"<< tables[i]<<"' in '" << file1 << "' not found in '" << file2 << "' or wrong unmatching number of tables.";
            throw Exception(error_ss.str());
        }
        i++;
    }
    sqlite3_finalize(statement);
    if(i!=(int)tables.size()){
        stringstream error_ss;
        error_ss << "merge: different number of tables in '" << file1 << "' and '" << file2 << "'.";
        throw Exception(error_ss.str());
    }
    if(ret!=SQLITE_DONE){
        cout << "error: " << ret << endl;
        throw Exception("");
    }

    //If we are here, the list of table names in file1 and file2 is the same. Go through the tables
    // and merge the result, if the schema matches:
    try{
        sqlite3_exec(db, "BEGIN");
        for(size_t itable=0; itable<tables.size(); itable++){
            ss.str("");
            ss << "INSERT INTO " << tables[itable] << " SELECT * FROM o." << tables[itable];
            sqlite3_exec(db, ss.str().c_str());
        }
        sqlite3_exec(db, "END");
    }
    catch(Exception & e){
        sqlite3_close(db);
        throw;
    }
    sqlite3_close(db);
}

/** searches path non-recursively for files matching pattern and fills the found files in \c files (prefixed with \c path).
 */
void find_files(const string & path, const string & pattern, vector<string> & files){
  if (!fs::exists(path)){
      stringstream ss;
      ss << "path '" << path << "' does not exist!";
      throw Exception(ss.str());
  }
  fs::directory_iterator end_itr;
  boost::regex pattern_regex(pattern);
  for (fs::directory_iterator itr(path); itr != end_itr; ++itr ) {
      if(boost::regex_search(itr->leaf(), pattern_regex)) {
          stringstream ss;
          ss << path;
          if(path[path.size()-1]!='/') ss << "/";
          ss << itr->leaf();
          files.push_back(ss.str());
    }
  }
}


int main(int argc, char** argv){
    po::options_description desc("Allowed options");
    desc.add_options()("help", "show help message")
    ("outfile", po::value<string>(), "output file of merging")
    ("verbose,v", "verbose output (with progress)")
    ("in-dir", po::value<string>(), "input directory (all files matching *.db there will be merged)");

    po::options_description hidden("Hidden options");

    hidden.add_options()
    ("in-file", po::value< vector<string> >(), "in file");

    po::positional_options_description p;
    p.add("in-file", -1);

    po::options_description cmdline_options;
    cmdline_options.add(desc).add(hidden);

    po::variables_map vm;
    po::store(po::command_line_parser(argc, argv).options(cmdline_options).positional(p).run(), vm);
    po::notify(vm);

    if(vm.count("help")){
        cout << desc << endl;
        return 0;
    }
    if(vm.count("outfile")==0){
        cerr << "please specify an output file with --outfile=..." << endl;
        return 1;
    }

    string outfile = vm["outfile"].as<string>();
    bool verbose = vm.count("verbose");

    string in_dir;
    if(vm.count("in-dir")){
        in_dir = vm["in-dir"].as<string>();
    }

    vector<string> input_files;
    if(vm.count("in-file")){
        input_files = vm["in-file"].as< vector<string> >();
    }

    //go through input dir and add *.db to input_files:
    if(in_dir!=""){
        try{
            find_files(in_dir, "\\.db$", input_files);
        }
        catch(Exception & e){
            cerr << "Error while adding files in input directory: " << e.message << endl;
            return 1;
        }
    }

    if(input_files.size() < 1){
        cerr << "no input files" << endl;
        return 1;
    }

    bool created_output = false;
    for(size_t i=0; i<input_files.size(); i++){
        if(not fs::is_regular_file(input_files[i])){
            cerr << "Input file '" << input_files[i] << "' not found (or not a file). Skipping." << endl;
            continue;
        }
        if(not created_output){
            //create output by copying:
            if(verbose){
                cout << "Copying '" << input_files[i] << "' to '" << outfile << "' ... "<< flush;
            }
            try{
                if(fs::exists(outfile)){
                    //cout << "Note: overwriting existing output file '" << outfile << "'" << endl;
                    fs::remove(outfile);
                }
                fs::copy_file(input_files[i], outfile);
            }
            catch(fs::filesystem_error & ex){
                cout << "error while copying '" << input_files[i] << "' to '" << outfile << "': " << ex.what() << endl;
                return 2;
            }
            if(verbose){
                cout << "done." << endl;
            }
            created_output = true;
        }
        else{
            if(verbose){
                cout << "Merging '" << input_files[i] << "' to '"<< outfile <<"' ... " << flush;
            }
            try{
                merge(outfile, input_files[i]);
            }
            catch(Exception & e){
                cerr << "Error while merging '" << input_files[i] << "': " << e.message << endl;
                cerr << "Aborting." << endl;
                exit(2);
            }
            if(verbose){
                cout << "done." << endl;
            }
        }
    }
    return 0;
}


