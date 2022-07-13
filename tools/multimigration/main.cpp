#include <cassert>
#include <fstream>
#include <iostream>
#include <random>
#include <sstream>
#include <thread>
#include <utility>
#include <vector>
#include <libpq-fe.h>
//---------------------------------------------------------------------------
// Copyright (c) 2022 TUM. All rights reserved.
//---------------------------------------------------------------------------
using namespace std;
//---------------------------------------------------------------------------
namespace {
//---------------------------------------------------------------------------
const vector<string> queries{
   "1.sql", "10.sql", "11.sql", "12.sql", "13.sql", "14a.sql", "14b.sql",
   "15.sql", "16.sql", "17.sql", "18.sql", "19.sql", "2.sql", "20.sql",
   "21.sql", "22.sql", "23a.sql", "23b.sql", "24a.sql", "24b.sql", "25.sql",
   "26.sql", "27.sql", "28.sql", "29.sql", "3.sql", "30.sql", "31.sql",
   "32.sql", "33.sql", "34.sql", "35.sql", "36.sql", "37.sql", "38.sql",
   "39a.sql", "39b.sql", "4.sql", "40.sql", "41.sql", "42.sql", "43.sql",
   "44.sql", "45.sql", "46.sql", "47.sql", "48.sql", "49.sql", "5.sql",
   "50.sql", "51.sql", "52.sql", "53.sql", "54.sql", "55.sql", "56.sql",
   "57.sql", "58.sql", "59.sql", "6.sql", "60.sql", "61.sql", "62.sql",
   "63.sql", "64.sql", "65.sql", "66.sql", "67.sql", "68.sql", "69.sql",
   "7.sql", "70.sql", "71.sql", "72.sql", "73.sql", "74.sql", "75.sql",
   "76.sql", "77.sql", "78.sql", "79.sql", "8.sql", "80.sql", "81.sql",
   "82.sql", "83.sql", "84.sql", "85.sql", "86.sql", "87.sql", "88.sql",
   "89.sql", "9.sql", "90.sql", "91.sql", "92.sql", "93.sql", "94.sql",
   "95.sql", "96.sql", "97.sql", "98.sql", "99.sql"};
//---------------------------------------------------------------------------
/// Status code for internal handling
enum StatusCode : unsigned { Success = 0,
                             Error = 1 };
//---------------------------------------------------------------------------
/// Postgres result wrapper
class PGResult {
   private:
   /// The postgres result
   pg_result* res;
   /// The expected success code
   int successCode;

   public:
   /// Constructor
   PGResult(pg_result* res, int successCode) : res(res), successCode(successCode) {}
   /// Destructor
   ~PGResult() {
      if (res)
         PQclear(res);
   }

   StatusCode checkStatus() const {
      assert(res);
      if (PQresultStatus(res) != successCode) {
         cerr << PQresultErrorMessage(res) << endl;
         return StatusCode::Error;
      }

      return StatusCode::Success;
   }
};
//---------------------------------------------------------------------------
class PGConnection {
   private:
   /// The postgres connection
   pg_conn* connection;

   public:
   /// Constructor
   PGConnection() : connection(nullptr) {}
   /// Destructor
   ~PGConnection() { disconnect(); }

   /// Connect to postgres
   StatusCode connect(const string& con) {
      disconnect();
      StatusCode res = StatusCode::Error;
      // Retry 5 times in a 100-millisecond interval
      for (auto i = 0; i < 5; i++) {
         connection = PQconnectdb(con.c_str());
         if (auto status = PQstatus(connection); status == CONNECTION_OK) {
            res = StatusCode::Success;
            break;
         }

         // Log issues
         cerr << PQerrorMessage(connection) << endl;

         disconnect();
         this_thread::sleep_for(100ms);
      }

      return res;
   }

   /// Disconnect from postgres
   void disconnect() {
      if (connection)
         PQfinish(connection);
      connection = nullptr;
   }

   /// Execute a query
   PGResult executeQuery(const string& query) {
      return PGResult{PQexec(connection, query.c_str()), PGRES_TUPLES_OK};
   }
   /// Execute a set command
   PGResult executeSet(const string& command) {
      return PGResult{PQexec(connection, command.c_str()), PGRES_COMMAND_OK};
   }
};
//---------------------------------------------------------------------------
/// Client responsible for executing a TPCDS query
class QueryClient {
   private:
   /// The query name
   const string queryName;
   /// The executing thread
   thread execThread;
   /// The connection to postgres
   PGConnection connection;
   /// The connection string
   const string connectionString;
   /// The query
   string query;
   /// The cancel flag
   atomic<bool>& cancel;

   public:
   /// Constructor
   QueryClient(string queryName, string connectionString, atomic<bool>& cancel) : queryName(move(queryName)), connectionString(move(connectionString)), cancel(cancel) {
      ifstream in(this->queryName);
      ostringstream sstr;
      sstr << in.rdbuf();
      query = sstr.str();
   }
   /// Destructor
   ~QueryClient() {}

   /// Initialize
   StatusCode initialize() { return connection.connect(connectionString); }

   /// Execute the query
   void performWork() {
      while (!cancel.load(memory_order_acquire)) {
         auto res = connection.executeQuery(query);

         if (res.checkStatus() == Error)
            cerr << "failed executing " << queryName << endl;
      }
   };

   /// Start execution
   void start() {
      execThread = thread([this]() { performWork(); });
   }

   /// Join the thread
   void join() { execThread.join(); }
};
//---------------------------------------------------------------------------
class BenchmarkClient {
   private:
   /// The query path
   const string queryPath;
   /// The connection string
   const string connectionString;
   /// The number of concurrent queries to run
   unsigned numQueries;
   /// The delay to wait before triggering migration
   unsigned migDelay;
   /// The number of experiments to run
   unsigned numRuns;
   /// The connection for control messages
   PGConnection connection;
   /// The cancellation flag for all query clients
   atomic<bool> cancel;

   public:
   /// Constructor
   BenchmarkClient(string queryPath, string connectionString, unsigned numQueries, unsigned numRuns) : queryPath(move(queryPath)), connectionString(move(connectionString)), numQueries(numQueries), numRuns(numRuns), cancel(false) {}
   /// Destructor
   ~BenchmarkClient() {}

   /// Run the benchmark
   int run() {
      ofstream logStream("migrationlog.csv", ios_base::app | ios_base::out);
      random_device rd;
      mt19937 gen(rd());

      for (auto i = 0; i < numRuns; i++) {
         vector<unique_ptr<QueryClient>> clients(numQueries);
         migDelay = uniform_int_distribution<unsigned>(10000, 30000)(gen);

         cout << "Run " << i << ", Delay " << migDelay / 1000.0 << "s" << endl;

         vector<string> qToRun;

         sample(queries.begin(), queries.end(), back_inserter(qToRun), numQueries, gen);

         for (auto j = 0; j < numQueries; j++) {
            clients[j] = make_unique<QueryClient>(queryPath + "/" + qToRun[j], connectionString, cancel);
            auto status = clients[j]->initialize();
            if (status == StatusCode::Error)
               return 1;
         }

         auto status = connection.connect(connectionString);
         if (status == StatusCode::Error)
            return 1;

         //connection.executeSet("SET DEBUG.MIGRATABLEQUERIES = 1;");
         connection.executeSet("SET DEBUG.MIGRATENOW = 0;");
         connection.executeSet("SET DEBUG.MIGRATIONKEY = '" + to_string(i) + "," + to_string(numQueries) + "'");

         cancel.store(false, memory_order_release);
         for (auto& client : clients) {
            client->start();
         }

         this_thread::sleep_for(chrono::milliseconds(migDelay));
         cancel.store(true, memory_order_release);
         connection.executeSet("SET DEBUG.MIGRATENOW = 1;");
         auto triggered = chrono::duration_cast<chrono::microseconds>(chrono::steady_clock::now().time_since_epoch()).count();

         logStream << i << "," << numQueries << ",TTT,," << triggered << endl;

         for (auto& client : clients)
            client->join();
      }
      logStream.close();
      return 0;
   }
};
//---------------------------------------------------------------------------
static int clientMain(int argc, char** argv) {
   if (argc < 5 || argc > 6) {
      cout << "Usage: " << endl;
      cout << "multimigration NumberOfRuns NumberOfQueries QueryPath ConnectionString" << endl;
      return 0;
   }
   auto numRuns = atoi(argv[1]);
   auto numQueries = atoi(argv[2]);
   string queryPath = argv[3];
   string connectionString = argv[4];

   BenchmarkClient client(queryPath, connectionString, numQueries, numRuns);
   return client.run();
}
//---------------------------------------------------------------------------
} // namespace
//---------------------------------------------------------------------------
int main(int argc, char** argv) {
   cout << "Starting Benchmark" << std::endl;
   return clientMain(argc, argv);
}
