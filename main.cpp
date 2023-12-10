#include <bits/stdc++.h>
using namespace std;
#if __has_include(<atcoder/all>)
    #include <atcoder/all>
using namespace atcoder;
#endif
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

inline unsigned int rand_int() {
    static unsigned int tx = 123456789, ty=362436069, tz=521288629, tw=88675123;
    unsigned int tt = (tx^(tx<<11));
    tx = ty; ty = tz; tz = tw;
    return ( tw=(tw^(tw>>19))^(tt^(tt>>8)) );
}

inline double rand_double() {
    return (double)(rand_int()%(int)1e9)/1e9;
}

//温度関数
#define TIME_LIMIT 2950
inline double temp(double start) {
    double start_temp = 100,end_temp = 1;
    return start_temp + (end_temp-start_temp)*((utility::mytm.elapsed()-start)/TIME_LIMIT);
}

//焼きなましの採用確率
inline double prob(int best,int now,int start) {
    return exp((double)(now - best) / temp(start));
}

//-----------------以下から実装部分-----------------//

#define DIR_NUM 4
// 上下左右の順番
vector<int> dx = {-1, 1, 0, 0};
vector<int> dy = { 0, 0,-1, 1};

struct Rect{
    int x, y, xl, yl, area;
    Rect() : x(0), y(0), xl(0), yl(0) {}
    explicit Rect(int _x, int _y, int _xl, int _yl) : x(_x), y(_y), xl(_xl), yl(_yl), area(_xl*_yl) {}
    bool operator<(const Rect& r) const {
        if( !(area%2) == !(r.area%2) ) return (area < r.area);
        return (area%2);
    }
};

struct Solver{
    string s;
    int n, xl, yl, dr, dc, nx, ny, d_total;
    vector<bool> vis_rect;
    vector<vector<bool>> vis;
    vector<vector<int>> d, rect, clean_turn;
    vector<vector<vector<bool>>> wall;

    vector<Rect> rects;
    vector<pair<int,int>> ans;

    Solver(){
        this->input();
        rect.assign(n+2,vector<int>(n+2,0));
        vis.assign(n+2,vector<bool>(n+2,true));
        clean_turn.assign(n+2,vector<int>(n+2,0));
        for(int i=1; i<=n; i++) for(int j=1; j<=n; j++) rect[i][j] = false, vis[i][j] = false;
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
        d_total = 0;
        for(int i=1; i<=n; i++) for(int j=1; j<=n; j++) {
            cin >> d[i][j];
            d_total += d[i][j];
        }
        return;
    }

    void output(){
        rep(i,ans.size()-1) {
            auto&& [x1,y1] = ans[i];
            auto&& [x2,y2] = ans[i+1];
            char ch = changeChar(x1,y1,x2,y2);
            cout << ch;
        }
        cout << '\n' << flush;
        return;
    }

    void solve(){

        // まずは貪欲解
        // 1. 縦横少なくとも一方が偶数の長方形を確保 (今回は評価値は面積)
        // 2. 長方形を繋げていく (長方形は少ない方が良い)
        // ※ 奇数*奇数 の長方形は 辺の長さが1 ⇒ その面積分余分に移動必要
        //            〃          どちらも2以上 ⇒ 2だけ余分に移動必要

        int rect_cnt = 0, rect_idx = 1;
        rects.emplace_back(Rect());
        
        priority_queue<Rect> pq;
        while( rect_cnt < n*n ) {
            for(int i=1; i<=n; i++) for(int j=1; j<=n; j++) {
                if( rect[i][j] ) continue;
                xl = yl = 0;
                expandRect(i,j,xl,yl);
                pq.push(Rect(i,j,xl,yl));
            }
            auto&& r = pq.top();
            rects.emplace_back(pq.top());
            rect_cnt += r.area;
            for(int i=r.x; i<r.x+r.xl; i++) for(int j=r.y; j<r.y+r.yl; j++) rect[i][j] = rect_idx;
            while( !pq.empty() ) pq.pop();
            rect_idx++;
        }
        rep(d,DIR_NUM) {
            rep(j,n+2) {
                rep(k,n+2) cerr << wall[j][k][d];
                cerr << endl;
            }
            cerr << endl;
        }
        cerr << endl;

        rep(i,n+2) {
            rep(j,n+2) cerr << rect[i][j] << " " << (rect[i][j] < 10 ? " " : "");
            cerr << '\n' << flush;
        }
        cerr << '\n' << flush;

        vis_rect.assign(rect_idx,false);
        vis_rect[0] = true;
        paintRect(rect[1][1],1,1);
        return;
    }

    inline void expandRect(int x, int y, int& xl, int& yl) {
        bool fr, fc;
        dr = dc = 1;
        while( dr != 0 || dc != 0 ) {
            // fr: 下に進行可能か, fc : 右に進行可能か
            fr = fc = true;
            for(int i=y; i<=y+yl; i++) {
                fr &= (!wall[x+xl][i][1] && !rect[x+xl+1][i]);
                if( i != y+yl ) fr &= (!wall[x+xl+1][i][3]);
            }
            if( !fr ) dr = 0;
            xl += dr;

            for(int i=x; i<=x+xl; i++) {
                fc &= (!wall[i][y+yl][3] && !rect[i][y+yl+1]);
                if( i != x+xl ) fc &= (!wall[i][y+yl+1][1]);
            }
            if( !fc ) dc = 0;
            yl += dc;
        }
        xl++, yl++;
        return;
    }

    void paintRect(int idx, int tx, int ty) {
        // 長方形塗りつぶし part (貪欲)
        // - 四隅のどこかに到達したら dfs っぽく次に行く感じ
        // ⇒ 縦長偶数 or 横長偶数 or それ以外 で分けて行き方をハードコード

        int px = tx, py = ty, ndir = (rects[idx].xl == 1 ? 2 : 0), cnt = 0;
        vis_rect[idx] = true;

        while( true ) {
            if( !vis[tx][ty] ) cnt++;
            vis[tx][ty] = true;
            clean_turn[tx][ty] = ans.size();
            ans.emplace_back(pair(tx,ty));

            rep(dir,DIR_NUM) {
                if( wall[tx][ty][dir] ) continue;
                nx = tx+dx[dir], ny = ty+dy[dir];
                
                if( vis_rect[rect[nx][ny]] ) continue; // 既に到達済みの長方形は continue
                paintRect(rect[nx][ny],nx,ny);
                clean_turn[tx][ty] = ans.size();
                ans.emplace_back(pair(tx,ty));
            }

            auto&& r = rects[idx];

            int bx = tx-r.x+1, by = ty-r.y+1;
            if( r.xl == 1 ) {
                // 横一列
                nx = tx+dx[ndir], ny = ty+dy[ndir];
                if( wall[tx][ty][ndir] || rect[nx][ny] != idx ) ndir = (ndir == 2 ? 3 : 2);
            }
            else if( r.yl == 1 ) {
                // 縦一列
                nx = tx+dx[ndir], ny = ty+dy[ndir];
                if( wall[tx][ty][ndir] || rect[nx][ny] != idx ) ndir = (ndir == 0 ? 1 : 0);
            }
            else if( r.xl%2 == 0 ) { // 縦偶数長 ⇒ 横くねくね
                if( by == 1 && bx != r.xl ) ndir = 1;
                else if( bx != 1 && ((by == 2 && bx%2 == 1) || (by == r.yl && bx%2 == 0)) ) ndir = 0;
                else if( bx%2 == 1 ) ndir = 2;
                else ndir = 3;
            }
            else { // 横偶数長 ⇒ 縦くねくね
                if( bx == 1 && by != r.yl ) ndir = 3;
                else if( by != 1 && ( (bx == 2 && by%2 == 1) || (bx == r.xl && by%2 == 0) ) ) ndir = 2;
                else if( by%2 == 1 ) ndir = 0;
                else ndir = 1;
            }
            if( cnt == rects[idx].area ) break;
            tx += dx[ndir], ty += dy[ndir];
        }
        // 縦横ともに奇数の時は仕方なく戻る
        while( tx != px || ty != py ) {
            if( ans.back() != pair(tx,ty) ) {
                clean_turn[tx][ty] = ans.size();
                ans.emplace_back(pair(tx,ty));
            }
            if( tx != px ) ndir = (tx < px);
            if( ty != py ) ndir = 2 + (ty < py);
            tx += dx[ndir], ty += dy[ndir];
        }
        if( ans.back() != pair(tx,ty) ) {
            clean_turn[tx][ty] = ans.size();
            ans.emplace_back(pair(tx,ty));
        }
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