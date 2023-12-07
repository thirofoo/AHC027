#include <bits/stdc++.h>
using namespace std;
#define rep(i, n) for(int i = 0; i < n; i++)
#define reps(i, s, n) for (int i = s; i < n; i++)

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
    double start_temp = 1000, end_temp = 10;
    return start_temp + (end_temp-start_temp)*((utility::mytm.elapsed()-start)/TIME_LIMIT);
}

//焼きなましの採用確率
inline double prob(long long best,long long now,int start) {
    return exp((double)(best-now) / temp(start));
}

//-----------------以下から実装部分-----------------//
using P = pair<int,int>;

#define DIR_NUM 4
// 上下左右の順番
vector<int> dx = {-1, 1, 0, 0};
vector<int> dy = { 0, 0,-1, 1};

struct Rect{
    int x, y, xl, yl, area;
    Rect() : x(0), y(0), xl(0), yl(0) {}
    explicit Rect(int _x, int _y, int _xl, int _yl) : x(_x), y(_y), xl(_xl), yl(_yl), area(_xl*_yl) {}
    bool operator<(const Rect& r) const {
        // 両者縦横いずれかに偶数長がある ⇒ 面積が大きい方が『真に大きい』
        // そうでない場合 ⇒ 偶数長がある長方形が『真に大きい』
        if( !(area%2) == !(r.area%2) ) return (abs(x-y) > abs(r.x-r.y));
        return (area%2);
    }
};

struct Solver{
    int n, xl, yl, dr, dc, nx, ny, d_total, rect_cnt, rect_idx, turn;
    long long best_score, cand_score;
    vector<P> ans;
    vector<Rect> rects;
    vector<bool> vis_rect, l_shape;
    vector<vector<bool>> vis;
    vector<vector<int>> d, rect, clean_turn, vis_cnt;
    vector<vector<vector<bool>>> wall;
    vector<vector<vector<int>>> history;

    Solver(){
        this->input();
        vis_rect.assign(rect_idx,false);
        vis.assign(n+2,vector(n+2,true));
        rect.assign(n+2,vector(n+2,0));
        vis_cnt.assign(n+2,vector(n+2,0));
        clean_turn.assign(n+2,vector(n+2,0));
        history.assign(n+2,vector(n+2,vector<int>{}));
        reps(i,1,n+1) reps(j,1,n+1) rect[i][j] = false, vis[i][j] = false;

        vis_rect[0] = true;
        turn = 0;
        utility::mytm.CodeStart();
    }

    void input(){
        cin >> n;
        d.assign(n+2,vector(n+2,0));
        wall.assign(n+2,vector(n+2,vector(4,true)));
        reps(i,1,n) {
            string s; cin >> s;
            reps(j,1,n+1) {
                wall[i][j][1] = s[j-1]-'0';
                wall[i+1][j][0] = s[j-1]-'0';
            }
        }
        reps(i,1,n+1) {
            string s; cin >> s;
            reps(j,1,n) {
                wall[i][j][3] = s[j-1]-'0';
                wall[i][j+1][2] = s[j-1]-'0';
            }
        }
        d_total = 0;
        reps(i,1,n+1) reps(j,1,n+1) {
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
        cerr << best_score / (ans.size()-1) << '\n';
        return;
    }

    void solve(){
        // まずは貪欲解
        // 1. 縦横少なくとも一方が偶数の長方形を確保 (今回は評価値は面積)
        // 2. 長方形を繋げていく (長方形は少ない方が良い)
        // ※ 奇数*奇数 の長方形は 辺の長さが1 ⇒ その面積分余分に移動必要
        //            〃          どちらも2以上 ⇒ 2だけ余分に移動必要

        selectRect();             // 長方形を選別
        walkRect(rect[1][1],1,1); // 初期解生成

        best_score = 0;
        reps(i,1,n+1)reps(j,1,n+1) {
            auto&& h = history[i][j];
            rep(k,h.size()) {
                int size = (k == h.size()-1 ? h[0]+ans.size()-1-h[k] : h[k+1]-h[k]);
                best_score += ((size-1)*d[i][j])*size/2;
            }
        }
        cerr << best_score / (ans.size()-1) << endl;

        moveRect();               // 焼き鈍しで初期解いじり

        return;
    }

    void selectRect() {
        rect_cnt = 0, rect_idx = 1;
        rects.emplace_back(Rect());
        priority_queue<Rect> pq;
        while( rect_cnt < n*n ) {
            reps(i,1,n+1) reps(j,1,n+1) {
                if( rect[i][j] ) continue;
                xl = yl = 0;
                expandRect(i,j,xl,yl);
                pq.push(Rect(i,j,xl,yl));
            }
            auto&& r = pq.top();
            rects.emplace_back(pq.top());
            rect_cnt += r.area;
            reps(i,r.x,r.x+r.xl) reps(j,r.y,r.y+r.yl) rect[i][j] = rect_idx;
            while( !pq.empty() ) pq.pop();
            rect_idx++;
        }
    }

    inline void expandRect(int x, int y, int& xl, int& yl) {
        dr = dc = 1;
        while( dr ) {
            reps(i,y,y+yl+1) {
                dr &= (!wall[x+xl][i][1] && !rect[x+xl+1][i]);
                if( i != y+yl ) dr &= (!wall[x+xl+1][i][3]);
            }
            xl += dr;
        }
        while( dc ) {
            reps(i,x,x+xl+1) {
                dc &= (!wall[i][y+yl][3] && !rect[i][y+yl+1]);
                if( i != x+xl ) dc &= (!wall[i][y+yl+1][1]);
            }
            yl += dc;
        }
        xl++, yl++;
        return;
    }

    void walkRect(int idx, int tx, int ty) {
        // 長方形塗りつぶし part (貪欲)
        // - 四隅のどこかに到達したら dfs っぽく次に行く感じ
        // ⇒ 縦長偶数 or 横長偶数 or それ以外 で分けて行き方をハードコード

        int px = tx, py = ty, ndir = (rects[idx].xl == 1 ? 2 : 0), cnt = 0;
        vis_rect[idx] = true;

        while( true ) {
            if( !vis[tx][ty] ) cnt++;
            vis[tx][ty] = true;
            ansPush(tx,ty);

            rep(dir,DIR_NUM) {
                if( wall[tx][ty][dir] ) continue;
                nx = tx+dx[dir], ny = ty+dy[dir];
                
                if( vis_rect[rect[nx][ny]] ) continue; // 既に到達済みの長方形は continue
                walkRect(rect[nx][ny],nx,ny);
                ansPush(tx,ty);
            }

            auto&& r = rects[idx];
            int bx = tx-r.x+1, by = ty-r.y+1;

            if( r.xl == 1 ) { // 横一列
                nx = tx+dx[ndir], ny = ty+dy[ndir];
                if( wall[tx][ty][ndir] || rect[nx][ny] != idx ) ndir = (ndir == 2 ? 3 : 2);
            }
            else if( r.yl == 1 ) { // 縦一列
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
            if( ans.back() != pair(tx,ty) ) ansPush(tx,ty);
            if( tx != px ) ndir = (tx < px);
            if( ty != py ) ndir = 2 + (ty < py);
            tx += dx[ndir], ty += dy[ndir];
        }
        if( ans.back() != pair(tx,ty) ) ansPush(tx,ty);
    }

    void moveRect() {
        // 予め初期解を何重にも重ねておく
        long long tmp_score = best_score;
        vector<P> tmp = ans;
        for(int i=1; i<ans.size(); i++) {
            auto&& [tx,ty] = ans[i];
            tmp.emplace_back(ans[i]);
            history[tx][ty].emplace_back(turn++);
            vis_cnt[tx][ty]++;
        }
        tmp_score += best_score;
        for(int i=1; i<ans.size(); i++) {
            auto&& [tx,ty] = ans[i];
            tmp.emplace_back(ans[i]);
            history[tx][ty].emplace_back(turn++);
            vis_cnt[tx][ty]++;
        }
        tmp_score += best_score;
        swap(tmp_score, best_score);
        swap(tmp, ans);
        
        // 初めにL字の3連単を列挙 (先頭のidxを保持)
        l_shape.assign(ans.size(),false);
        rep(i,ans.size()-1) {
            auto&& [x1,y1] = ans[i];
            auto&& [x2,y2] = ans[(i+1)%ans.size()];
            auto&& [x3,y3] = ans[(i+2)%ans.size()];
            if( (abs(x1-x2) == 1 && abs(y2-y3) == 1) || (abs(y1-y2) == 1 && abs(x2-x3) == 1) ) {
                l_shape[i] = true;
            }
        }

        // 山登り法
        while( utility::mytm.elapsed() <= TIME_LIMIT ) {
            // 近傍 : Ｌ型の経路を反転 (差分更新がO(1)) 
            int rnd = rand_int()%ans.size();
            if( !l_shape[rnd] ) continue;
            
            cand_score = diffScore(rnd);

            // if( cand_score < best_score ) {
            if( prob(best_score/(ans.size()-1), cand_score/(ans.size()-1), 0) > rand_double() ) {
                cerr << cand_score/(ans.size()-1) << " " << best_score/(ans.size()-1) << '\n';
                // int num = 0;
                // rep(i,ans.size()) num += l_shape[i];
                // cerr << num << '\n';
                best_score = cand_score;
                // 差分更新
                auto&& [x1,y1] = ans[rnd];
                auto   [x2,y2] = ans[(rnd+1)%ans.size()];
                auto&& [x3,y3] = ans[(rnd+2)%ans.size()];
                history[x2][y2].erase(find(history[x2][y2].begin(), history[x2][y2].end(), rnd+1));
                vis_cnt[x2][y2]--;

                if     ( (x1-x2 == -1 && y3-y2 ==  1) || (y1-y2 ==  1 && x3-x2 == -1) ) x2--, y2++; // 左下凸
                else if( (x1-x2 ==  1 && y3-y2 ==  1) || (y1-y2 ==  1 && x3-x2 ==  1) ) x2++, y2++; // 左上凸
                else if( (x1-x2 == -1 && y3-y2 == -1) || (y1-y2 == -1 && x3-x2 == -1) ) x2--, y2--; // 右下凸
                else                                                                    x2++, y2--; // 右上凸

                ans[(rnd+1)%ans.size()] = pair(x2,y2);
                vis_cnt[x2][y2]++;
                history[x2][y2].emplace_back(rnd+1);
                sort(history[x2][y2].begin(), history[x2][y2].end());

                // L字消滅 or 新候補
                l_shape[rnd-1] = !l_shape[rnd-1];
                l_shape[rnd+1] = !l_shape[rnd+1];
            }
        }
    }

    inline long long diffScore(const int& rnd) {
        long long res = best_score;
        auto&& [x1,y1] = ans[rnd];
        auto   [x2,y2] = ans[(rnd+1)%ans.size()];
        auto&& [x3,y3] = ans[(rnd+2)%ans.size()];
        if( vis_cnt[x2][y2] == 1 ) return 1e17;

        int size2 = 0;
        auto&& h1 = history[x2][y2];
        auto itr = lower_bound(h1.begin(), h1.end(), rnd+1);
        int size = (( itr != h1.end() ? *itr : *h1.begin() ) - ( itr != h1.begin() ? *(itr-1) : *h1.rbegin() ) + ans.size()-1)%(ans.size()-1);
        res -= ((size-1)*d[x2][y2])*size/2;
        size2 += size;

        if( itr != h1.end() ) itr++;
        else itr = h1.begin()+1;
        size = (( itr != h1.end() ? *itr : *h1.begin() ) - ( itr != h1.begin() ? *(itr-1) : *h1.rbegin() ) + ans.size()-1)%(ans.size()-1);
        res -= ((size-1)*d[x2][y2])*size/2;
        size2 += size;

        res += ((size2-1)*d[x2][y2])*size2/2;

        if( (x1-x2 == -1 && y3-y2 ==  1) || (y1-y2 ==  1 && x3-x2 == -1) ) { // 左下凸
            if( wall[x1][y1][3] || wall[x3][y3][0] || wall[x1][y1][0] || wall[x3][y3][3] ) return 1e17;
            x2--, y2++; 
        }
        else if( (x1-x2 ==  1 && y3-y2 ==  1) || (y1-y2 ==  1 && x3-x2 ==  1) ) { // 左上凸
            if( wall[x1][y1][3] || wall[x3][y3][1] || wall[x1][y1][1] || wall[x3][y3][3] ) return 1e17;
            x2++, y2++; 
        }
        else if( (x1-x2 == -1 && y3-y2 == -1) || (y1-y2 == -1 && x3-x2 == -1) ) { // 右下凸
            if( wall[x1][y1][0] || wall[x3][y3][2] || wall[x1][y1][2] || wall[x3][y3][0] ) return 1e17;
            x2--, y2--;
        }
        else { // 右上凸 
            if( wall[x1][y1][1] || wall[x3][y3][2] || wall[x1][y1][2] || wall[x3][y3][1] ) return 1e17;
             x2++, y2--; 
        }

        size2 = 0;
        auto&& h2 = history[x2][y2];
        itr = lower_bound(h2.begin(), h2.end(), rnd+1);
        size = (( itr != h2.end() ? *itr : *h2.begin() ) - ( itr != h2.begin() ? *(itr-1) : *h2.rbegin() ) + ans.size()-1)%(ans.size()-1);
        res -= ((size-1)*d[x2][y2])*size/2;

        size = (( itr != h2.end() ? *itr : *h2.begin() ) - (rnd+1) + ans.size()-1)%(ans.size()-1);
        res += ((size-1)*d[x2][y2])*size/2;
        size = ((rnd+1) - ( itr != h2.begin() ? *(itr-1) : *h2.rbegin() ) + ans.size()-1)%(ans.size()-1);
        res += ((size-1)*d[x2][y2])*size/2;

        return res;
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

    inline void ansPush(int tx, int ty) {
        ans.emplace_back(pair(tx,ty));
        clean_turn[tx][ty] = ans.size();
        history[tx][ty].emplace_back(turn++);
        vis_cnt[tx][ty]++;
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