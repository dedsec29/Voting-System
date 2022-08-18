#include <bits/stdc++.h>
using namespace std;
#define int long long

int const mod = 1e9+7;

int poww(int a,int b) {
    a %= mod;
    int res = 1;
    while (b) {
        if (b&1)
            res = res * a % mod;
        a = a * a % mod;
        b >>= 1;
    }
    return res;
}

class Config {
private:
    const int N = 1000;
    vector<int> unChosen;
    vector<int> PI; // Key
    mt19937 rng;

    void precomputation() {
        for (int i = 0; i < N; i++)
            unChosen[i] = i;
        PI = unChosen;
        shuffle(PI.begin(), PI.end(), rng);
    }

public:
    Config() {
        unChosen.resize(N);
        mt19937 RNG(chrono::steady_clock::now().time_since_epoch().count());
        rng = RNG;
        precomputation();
    }

    int get_id() {
        int idx = uniform_int_distribution<int>(0, unChosen.size()-1)(rng);
        int e = unChosen[idx];
        swap(unChosen[idx], unChosen[unChosen.size()-1]);
        unChosen.pop_back();
        return e;
    }

    int getPermutedNum(int id) {
        return PI[id];
    }
};

class Security {
public:
    int Encrypt_Adhar(int x) {
        for (int i = 0; i < 20; i += 1)
            x ^= 1<<i;
        return x;
    }

    int Encrypt_Choice(int x) {
        for (int i = 0; i < 20; i += 1)
            x ^= 1<<i;
        return x;
    }

    int Hash_Pair(int x,int y) {
        return ((x * y) % mod * poww(x, y)) % mod;
    }

    int Decrypt_Choice(int x) {
        for (int i = 0; i < 20; i += 1)
            x ^= 1<<i;
        return x;
    }

    int Hash_State(unordered_map<int,int>& mp) {
        vector<pair<int,int>> freq;
        for (auto i: mp)
            freq.emplace_back(i);
        sort(freq.begin(), freq.end());
        int A = 1, B = 0;
        int ptr = 1;
        for (auto i: freq) {
            A = (A * poww(i.first, ptr)) % mod;
            B = (B + poww(i.second, ptr + 2)) % mod;
            ptr++;
        }
        return (A + B) % mod;
    }

    int Hash_Combine(int x,int y) {
        return ((x^y) + ((x*y)%mod)) % mod;
    }
};

Security mySecurity;

// Persistent Linked List stored on disk
// (Used as save point, after crash if heap memory lost)
class PLL {
private:
    vector<int> chain;  // each node contains prehash

public:
    bool isEmpty() {
        return chain.size() == 0;
    }
    int getLastElement() {
        if (isEmpty())
            return -1;
        return chain[chain.size() - 1];
    }
    void insertElement(int value) {
        chain.push_back(value);
    }
};

// Runtime heap memory disk
struct node {
    int preHash;
    node* next;

    node() {
        next = nullptr;
    }

    node(int x) {
        next = nullptr;
        preHash = x;
    }
};

node* tail = nullptr;
PLL myPLL;

class DB_User_Vote {
private:
    unordered_map<int, int> db_votes;    // wrong_id, choice;// choice is encrypted incoming save query
    unordered_map<int, int> timer;       // wrong id => Sequential Index
    unordered_set<int> taken;            // set of encrypted user ids
    unordered_map<int, int> mappedIndex; // {encrypted_id->encrypted_option}
    unordered_map<int, int> freq;        // current state (encrypted choice, frequency)

public:
    bool add_vote(int x, int y) {
        if (db_votes.find(x) != db_votes.end())
            return false;
        db_votes[x] = y;
        freq[y]++;
        timer.insert({x, timer.size()+1});
        int current_hash;

        if (tail == nullptr) {
            // either first vote, or CRASH
            if (myPLL.isEmpty()) {
                // first vote
                current_hash = mySecurity.Hash_State(freq);
                tail = new node();
                tail->preHash = current_hash;
            }
            else {
                // CRASH. Recover from PLL
                tail = new node(myPLL.getLastElement());
                node* new_node = new node(mySecurity.Hash_Combine(tail->preHash, mySecurity.Hash_State(freq)));
                tail->next = new_node;
                tail = tail->next;
            }
        }
        else {
            node* new_node = new node(mySecurity.Hash_Combine(tail->preHash, mySecurity.Hash_State(freq)));
            tail->next = new_node;
            tail = tail->next;
        }
        myPLL.insertElement(tail->preHash);
        return true;
    }

    int get_choice(int x) {
        if (db_votes.find(x) == db_votes.end())
            return -1;
        return db_votes[x];
    }

    int from_scratch_hash() {
        vector<pair<int,int>> voters;       // wrong_id, choice
        for (auto i: db_votes)
            voters.emplace_back(i);
        sort(voters.begin(), voters.end(), [&](pair<int,int> a,pair<int,int> b) {
            return timer[a.first] < timer[b.first];
        });

        // Construct the hash now
        unordered_map<int,int> freq;
        freq[voters[0].second]++;

        int h = mySecurity.Hash_State(freq);
        for (int i = 1; i < (int)voters.size(); i++) {
            freq[voters[i].second]++;
            h = mySecurity.Hash_Combine(h, mySecurity.Hash_State(freq));
        }
        return h;
    }

    unordered_map<int, int> getVoteSummary() {
        return freq;
    }

    bool checkIfEncryptedIdIsUsed(int encrypted_id) {
        return taken.find(encrypted_id) != taken.end();
    }

    void UseEncryptedId(int encrypted_id) {
        taken.insert(encrypted_id);
    }

    void mapEncryptedIdWithNumber(int encrypted_id, int num) {
        mappedIndex[encrypted_id] = num;
    }

    int getNumFromEncryptedId(int encrypted_id) {
        return mappedIndex[encrypted_id];
    }
};

Config myConfig;
DB_User_Vote myDB_User_Vote;

void User_Vote_Front(int& A_id,int& choice) {
    // Enter Adhar
    cout << "Enter Adhar ID: \n";
    cin >> A_id;

    cout << "Enter your choice: \n";
    cin >> choice;
}

int User_Vote_Back(int &A_id, int &choice) {
    A_id = mySecurity.Encrypt_Adhar(A_id);
    choice = mySecurity.Encrypt_Choice(choice);
    int ccode = mySecurity.Hash_Pair(A_id, choice);
    if (myDB_User_Vote.checkIfEncryptedIdIsUsed(A_id))
        return -1;
    myDB_User_Vote.UseEncryptedId(A_id);
    int id = myConfig.get_id();
    myDB_User_Vote.mapEncryptedIdWithNumber(A_id, id);
    int new_id = myConfig.getPermutedNum(id);
    // now save (new_id, choice)
    if (!myDB_User_Vote.add_vote(new_id, choice))
        return -1;
    return ccode;
}

void Ensure_User_Vote_Sanctity_Front(int& A_id) {
    cout << "Enter your Adhar ID: \n";
    cin >> A_id;
    A_id = mySecurity.Encrypt_Adhar(A_id);
}

int Ensure_User_Vote_Sanctity_Back(int &E_id) {
    int wrong_id = myConfig.getPermutedNum(myDB_User_Vote.getNumFromEncryptedId(E_id));
    int choice = myDB_User_Vote.get_choice(wrong_id);
    if (choice == -1)
        return -1;
    int ccode = mySecurity.Hash_Pair(E_id, choice);
    return ccode;
}

void Display_Aggregate_Results() {
    //  party/choice , frequency
    unordered_map<int, int> freq = myDB_User_Vote.getVoteSummary();
    cout << "Aggregate Results: \n";
    for (auto i: freq)
        cout << mySecurity.Decrypt_Choice(i.first) << " " << i.second << "\n";
    cout << "\n";
}

void Trigger() {
    if (tail == NULL) {
        if (myPLL.isEmpty())
            cout << "Votes intact\n";
        else
            cout << "Perform recovery\n";
        return;
    }
    int H = tail->preHash;
    // Calculate H'
    int Hp = myDB_User_Vote.from_scratch_hash();
    if (H == Hp) cout << "Votes intact\n";
    else cout << "VOTES TAMPERED\n";
}

int32_t main() {
    // (1) CLI Adhar ID and choice
    // (2) User vote Sanctity
    // (3) Check votes won by parties so far
    // (4) Vote Sanctity check Admin side
    while(true) {
        cout << "*********************** \n";
        cout << "Enter your choice: \n";
        cout << "1. Vote \n";
        cout << "2. User Vote Safety \n";
        cout << "3. Admin Vote Consistency \n";
        cout << "4. Display aggregate results \n";
        cout << "5. Close the system \n";
        cout << "*********************** \n";
        int type; cin >> type;
        if (type == 1) {
            int A_id, choice;
            User_Vote_Front(A_id, choice);
            // Now, we inserted ( E(A_id), E(choice) )
            int confirmationCode = User_Vote_Back(A_id, choice);
            if (confirmationCode == -1) {
                cout << "Duplicate ID entry" << endl;
            }
            else {
                cout << "Confirmation Code: " << confirmationCode << endl;
                cout << "Please keep this for future reference." << endl;
            }
        }
        else if (type == 2) {
            int A_id;
            Ensure_User_Vote_Sanctity_Front(A_id);
            int confirmationCode = Ensure_User_Vote_Sanctity_Back(A_id);
            if (confirmationCode == -1) {
                cout << "No such Vote found" << endl;
            }
            else {
                cout << "Your confirmation code is " << confirmationCode << "\n";
            }
            cout << "If confirmation code does not match, raise alarm\n";
        }
        else if (type == 3) {
            Trigger();
        }
        else if (type == 4) {
            Display_Aggregate_Results();
        }
        else if (type == 5) {
            break;
        }
        else {
            cout << "Enter Correct Data" << endl;
        }
        cout << "Thank You!" << endl;
    }
    return 0;
}