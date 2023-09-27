#include <iostream>
#include <fstream>
#include <utility>
#include <algorithm>
#include <cmath>
#include <chrono>
#include "hw.h"

using namespace std;

int main()
{
    map<int, map<int, double>> user_map = create_map("train.csv", true);
    map<int, map<int, double>> movie_map = create_map("train.csv", false);
    
    // Print top 10 users
    vector<pair<int,int>> top_10_users = get_top_ten(user_map);
    cout << endl << "Top 10 Users" << endl;
    cout << "User - #Ratings" << endl;
    for(auto i:top_10_users)
        cout << "User" << i.first << " - " << i.second << endl;
    
    // Print top 10 movies
    vector<pair<int,int>> top_10_movies = get_top_ten(movie_map);
    cout << endl << "Top 10 Movies" << endl;
    cout << "Movie - #Ratings" << endl;
    for(auto i:top_10_movies)
        cout << "Movie" << i.first << " - " << i.second << endl;

    // Read test csv
    vector<pair<int, pair<int, int>>> test = read_test("test.csv");

    // Predict
    ofstream submission("submission.csv");
    submission << "ID,Predicted" << endl;
    auto start = chrono::high_resolution_clock::now(); // start time 
    for(auto i: test)
    {
        submission << i.first << ',';
        double prediction = CF(user_map, movie_map, i.second.first, i.second.second, K); 
        submission << prediction << endl;       
    }
    auto stop = chrono::high_resolution_clock::now(); // stop time
    auto duration = chrono::duration_cast<chrono::seconds>(stop - start);
    cout << "Test executed for a total of " << duration.count() << " seconds." << endl;
    submission.close();
    return 0;
}

map<int, map<int, double>>  create_map(string filename, bool usermode) // creates a map from input file (user mode or movie mode)
{
    ifstream file(filename);
    string item;
    int userid, movieid;
    double score;
    map<int, map<int, double>> result;
    getline(file, item); item.clear(); // discard header
    while(!file.eof())
    {
        getline(file, item, ',');
        if(file.eof())break;
        userid = stoi(item);
        getline(file, item, ',');
        movieid = stoi(item);
        getline(file, item, '\n');
        score = stod(item);
        if(usermode) result[userid][movieid] = score;
        else result[movieid][userid] = score;
    }
    file.close();
    return result;
}

vector<pair<int,int>> get_top_ten(map<int, map<int, double>> my_map) // returns top 10 for a map (user or movie)
{
    vector<pair<int,int>> result;
    for(auto iter : my_map)
        result.emplace_back(pair<int,int>(iter.first, iter.second.size()));

    sort(result.begin(), result.end(), [](const pair<int,int> &a, const pair<int,int> &b) {return a.second > b.second;});
    return vector<pair<int,int>> (result.begin(), result.begin()+10);
    
}

double ln_distance(vector<double> a, vector<double> b , double n) 
// Metric: Ln Distance (no root beucase it is irrelevant when comparing distances)
{
    double distance = 0;
    for(int i=0; i<a.size(); i++)
        distance += pow(a[i] - b[i], n);
    return distance;
}

vector<pair<int, pair<int, int>>> read_test(string filename) // read test file
{
    ifstream file(filename);
    string item;
    int id, userid, movieid;
    vector<pair<int, pair<int, int>>>  result;
    getline(file, item); item.clear(); // discard header
    while(!file.eof())
    {
        getline(file, item, ',');
        if(file.eof())break;
        id = stoi(item);
        getline(file, item, ',');
        userid = stoi(item);
        getline(file, item, '\n');
        movieid = stoi(item);
        result.emplace_back(pair<int, pair<int, int>>(id, pair<int, int>(userid, movieid)));
    }
    file.close();
    return result;
}

double CF_algorithm(map<int, map<int, double>>& my_map, vector<int>& rated, int guesser_id, int to_guess_id, int k)
// This implements both UBCF and IBCF. The only difference between them is the different map and order or user and movie ids
// Both method do exactly the same thing, when the map is transposed, so with this function we can do both.
// For UBCF -> my_map = user_map, rated is other users that rated this movie, guesser is user, to_guess is the movie
// For IBCF -> my_map = movie_map, rated is other movies rated by user, guesser is movie, to_guess is the user id
{
    vector<double> base_vector; // base vector to use for similarity check
    for(auto movie : my_map[guesser_id])
            base_vector.push_back(movie.second); // add the score
    
    vector<pair<int, double>> similarities;
    for(auto i: rated) // for all values in rated (UBCF->users that rated the movie, IBCF->movies rated by user)
    {
        vector<double> other_base_vector;
        for(auto rating : my_map[guesser_id])
        {
            // here, we get the common values (like common movies rated by two users),
            // if a value exists in base but doesnt exist in the vector to compare, we take it as 0
            // this acts as a projection to base_vectors subspace
            if(my_map[i].count(rating.first)>0) other_base_vector.push_back(my_map[i][rating.first]); // get the value
            else other_base_vector.push_back(DEFAULT_VALUE); // if not rated, take it as DEFAULT_VALUE
        }
        double similarity = (-1) * ln_distance(base_vector, other_base_vector, LN_VALUE); // l2 distance, -1 because lower is better
        similarities.emplace_back(pair<int, double>(i, similarity));// add to similarities
    }
    if(k==-1) k = (int) sqrt(similarities.size()); // this mode is for k=sqrt(n)
    if(similarities.size() < k) k = similarities.size(); //not enough users for k, use all the users
    
    sort(similarities.begin(), similarities.end(), [](const pair<int,double> &a, const pair<int,double> &b) {return a.second > b.second;});
    similarities = vector<pair<int, double>>(similarities.begin(), similarities.begin()+k); // get k top users

    double result = 0;
    for(auto i : similarities) // sum over top K users
    {
        result += my_map[i.first][to_guess_id];
    }
    result = result / k; // average
    return result;
}


double CF(map<int, map<int, double>>& user_map, map<int, map<int, double>>& movie_map, int user_id, int movie_id, int k)
// This is just a helper function that calls the CF algorithm. Some movies have no ratings by users, and some users have never
// rated any movies, in these cases we switch between UBCF and IBCF. 
{
    vector<int> rated_movie; // users who rated the movie
    for(auto i : movie_map[movie_id])
        rated_movie.push_back(i.first);
    
    vector<int> user_rated; // movies rated by the user
    for(auto i : user_map[user_id])
        user_rated.push_back(i.first);

    if(rated_movie.size()==0 && user_rated.size()==0) 
        // if nobody else has ever rated this movie and user has no ratings, it is impossible to make a choice. 
        // we just return a mean movie rating
        return MEAN;
    if(rated_movie.size() >= user_rated.size()) // if we have more data for UBCF
        return CF_algorithm(user_map, rated_movie, user_id, movie_id, k); // UBCF
    else // if we have more data for IBCF
        return CF_algorithm(movie_map, user_rated, movie_id, user_id, k); // ICBF
}