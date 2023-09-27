#define DEFAULT_VALUE 0 // Value to use when a point doesnt exist. Example: user1 has rated movie2 but user2 didnt, 
                        // to check similarity we need to be in the same subspace. Values between 0 and 5
#define K 25 // K value is used for taking the average rating of "k most similar" users/items. use -1 for k=sqrt(n)
#define MEAN 3.47 // Mean rating is used only for movies with no reviews AND if that user also never reviewed a movie (no data for UBCF AND IBCF)
#define LN_VALUE 2 // N value for the distance metric. L2 norm is used for the tests

#include <vector>
#include <string>
#include <map>

using namespace std;

map<int, map<int, double>>  create_map(string filename, bool usermode);
vector<pair<int,int>> get_top_ten(map<int, map<int, double>> my_map);
double ln_distance(vector<double> a, vector<double> b , double n) ;
vector<pair<int, pair<int, int>>> read_test(string filename);
double CF_algorithm(map<int, map<int, double>>& my_map, vector<int>& rated, int guesser_id, int to_guess_id, int k);
double CF(map<int, map<int, double>>& user_map, map<int, map<int, double>>& movie_map, int user_id, int movie_id, int k);
