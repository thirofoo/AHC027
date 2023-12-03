#include <bits/stdc++.h>
using namespace std;
#define rep(i, n) for(int i = 0; i < n; i++)

namespace utility {
    struct timer {
        chrono::system_clock::time_point start;
        // 開始時間を記録
        void CodeStart() {
            start = chrono::system_clock::now();
        }
        // 経過時間 (ms) を返す
        double elapsed() const {
        using namespace std::chrono;
            return (double)duration_cast<milliseconds>(system_clock::now() - start).count();
        }
    } mytm;
}
#define TIME_LIMIT 100000

//-----------------以下から実装部分-----------------//

#define DIR_NUM 4
// 上下左右の順番
vector<int> dx = {-1, 1, 0, 0};
vector<int> dy = { 0, 0,-1, 1};

int n;
vector<vector<int>> d;

template <class S>
struct Zobrist_hash_set {
    public:
    Zobrist_hash_set() : v(0) {
        mt.seed(rand());
        rnd = uniform_int_distribution<long long>(-LLONG_MAX, LLONG_MAX);
    }
    void flip(const S& x) { // hash flip
        if (!x_to_hash.count(x)) x_to_hash[x] = rnd(mt);
        v ^= x_to_hash[x];
    }
    void init() { v = 0; } // hash初期化
    void set(long long _v) { v = _v; } // hash set
    long long get() { return v; } // 現時点の状態hash値を返す．
    
    private:
    long long v; // hash値
    mt19937_64 mt;
    uniform_int_distribution<long long> rnd;
    unordered_map<S, long long> x_to_hash; // 各 x ∈ X に対するハッシュの割当
};
Zobrist_hash_set<int> zh;

struct State{
    bool isDone;
    char action;
    long long hash;
    int score, x, y, cnt;
    vector<long long> vis; // bit で訪問済みかを持っておく
    State() : score(0), hash(0LL) {}
    explicit State(const State& pre, const int dir) {
        x = pre.x + dx[dir], y = pre.y + dy[dir];
        vis = pre.vis;
        // 各マス (n*n) * 現在地か否か で Zobrist Hash
        zh.set( pre.hash );
        zh.flip( pre.x*(n+2)+pre.y + (n+2)*(n+2) );
        zh.flip( pre.x*(n+2)+pre.y );
        if( vis[x] & (1LL << y) ) zh.flip( x*(n+2)+y );
        zh.flip( x*(n+2)+y + (n+2)*(n+2) );

        action = changeChar(dir);
        vis[x] |= (1LL << y);
        hash = zh.get();
        calsScore();
    }

    inline void calsScore() {
        score = 0, cnt = 0;
        for(int i=1; i<=n; i++) {
            for(int j=1; j<=n; j++) {
                if( (vis[i] & (1LL << j)) || abs(i-x)+abs(j-y) == 0 ) continue;
                score += d[i][j] / (abs(i-x)+abs(j-y));
                cnt++;
            }
        }
    }
    inline char changeChar(int dir) {
        if     ( dir == 0 ) return 'U';
        else if( dir == 1 ) return 'D';
        else if( dir == 2 ) return 'L';
        else                return 'R';
    }
    bool operator<(const State& s) const {
        if( cnt == s.cnt ) return (score < s.score);
        return (cnt > s.cnt);
    }
};

struct Solver{
    int now_x, now_y, nx, ny, best_score;
    State state;
    string s, ans;
    long long best_hash, init_hash;
    vector<vector<bool>> vis;
    vector<vector<int>> clean_turn;
    vector<vector<vector<bool>>> wall;
    map<long long, pair<long long, char>> parent;
    
    Solver(){
        this->input();
        vis.assign(n+2,vector<bool>(n+2,true));
        clean_turn.assign(n+2,vector<int>(n+2,0));
        for(int i=1; i<=n; i++) for(int j=1; j<=n; j++) vis[i][j] = false;

        // State 初期化
        state.vis = vector<long long>(n+2,0);
        state.x = 1, state.y = 1;
        zh.init(), zh.flip( state.x*(n+2)+state.y + (n+2)*(n+2) );
        state.hash = zh.get();
        init_hash = state.hash;
        best_hash = 0;
        state.calsScore();
    }

    void input(){
        cin >> n;
        d.assign(n+2,vector<int>(n+2,0));
        wall.assign(n+2,vector(n+2,vector<bool>(4,true)));
        for(int i=1; i<=n-1; i++) {
            cin >> s;
            for(int j=1; j<=n; j++) {
                wall[i][j][1] = s[j-1]-'0';
                wall[i+1][j][0] = s[j-1]-'0';
            }
        }
        for(int i=1; i<=n; i++) {
            cin >> s;
            for(int j=1; j<=n-1; j++) {
                wall[i][j][3] = s[j-1]-'0';
                wall[i][j+1][2] = s[j-1]-'0';
            }
        }
        for(int i=1; i<=n; i++) for(int j=1; j<=n; j++) cin >> d[i][j];
        return;
    }

    void output(){
        cout << ans << '\n' << flush;
        return;
    }

    void solve(){
        // 深さ 10000, 幅 1 の chokudai search で愚直判定 O(N^2) でやってみる
        // 評価値 : ∑ (未訪問 ? d*turn : 0) / (Manhattan)
        chokudaiSearch(state, 1, 10000, TIME_LIMIT);
        convertAnswer(); // hash で最善手を復元
        returnToStart(); // Start地点に戻る
        return;
    }

    void chokudaiSearch(
        const State& state,
        const int beam_width,
        const int beam_depth,
        const int time_limit
    ) {
        utility::mytm.CodeStart();
        vector<priority_queue<State>> beam(beam_depth+1);
        rep(i,beam_depth+1) beam[i] = priority_queue<State>();
        beam[0].emplace(state);

        int max_depth = 0;

        while( utility::mytm.elapsed() <= TIME_LIMIT ) {
            rep(t,beam_depth) {
                auto &now_beam = beam[t];
                auto &next_beam = beam[t+1];
                rep(i,beam_width) {
                    if( now_beam.empty() ) break;
                    State now_state = now_beam.top();
                    now_beam.pop();
                    rep(d,DIR_NUM) {
                        if( wall[now_state.x][now_state.y][d] ) continue;
                        State next_state = State(now_state, d);
                        if( parent.count(next_state.hash) ) continue;
                        // cerr << now_state.x << " " << now_state.y << " " << now_state.hash << " " << next_state.hash << " " << next_state.action << '\n' << flush;

                        parent[next_state.hash] = pair(now_state.hash, next_state.action);
                        if( next_state.cnt == 0 ) {
                            best_hash = next_state.hash;
                            now_x = next_state.x, now_y = next_state.y;
                            cerr << "Yeah! " << best_hash << '\n';
                            cerr << now_x << " " << now_y << endl;
                            return;
                        }
                        next_beam.emplace(next_state);
                    }
                }
            }
        }
    }

    void returnToStart() {
        queue<tuple<int,int,int,int>> todo;
        vector<vector<bool>> visited(n+2,vector<bool>(n+2,false));
        vector<vector<pair<int,int>>> pre(n+2,vector<pair<int,int>>(n+2,pair(-1,-1)));
        todo.push(tuple(1,1,-1,-1));
        while( !todo.empty() ) {
            auto [tx,ty,px,py] = todo.front(); todo.pop();
            if( visited[tx][ty] ) continue;
            visited[tx][ty] = true;
            pre[tx][ty] = pair(px,py);
            rep(dir,DIR_NUM) {
                nx = tx+dx[dir], ny = ty+dy[dir];
                if( visited[nx][ny] || wall[tx][ty][dir] ) continue;
                todo.push(tuple(nx,ny,tx,ty));
            }
        }
        while( now_x != 1 || now_y != 1 ) {
            auto [px,py] = pre[now_x][now_y];
            cerr << now_x << " " << now_y << endl;
            ans += changeChar(now_x,now_y,px,py);
            cerr << changeChar(now_x,now_y,px,py) << endl;
            now_x = px, now_y = py;
        }
    }

    void convertAnswer() {
        cerr << "init_hash: " << init_hash << ", best_hash: " << best_hash << '\n';
        while( best_hash != init_hash ){
            // cerr << best_hash << " : " << parent[best_hash].first << " " << parent[best_hash].second << '\n' << flush;

            ans += parent[best_hash].second;
            best_hash = parent[best_hash].first;
        }
        reverse(ans.begin(), ans.end());
    }

    inline bool outField(int x,int y){
        if(1 <= x && x <= n && 1 <= y && y <= n)return false;
        return true;
    }
    inline char changeChar(int x1, int y1, int x2, int y2) {
        if     ( x2-x1 ==  1 ) return 'D';
        else if( x2-x1 == -1 ) return 'U';
        else if( y2-y1 ==  1 ) return 'R';
        else                   return 'L';
    }
};

int main(){
    cin.tie(0);
    ios_base::sync_with_stdio(false);

    Solver solver;
    solver.solve();
    solver.output();
    
    return 0;
}