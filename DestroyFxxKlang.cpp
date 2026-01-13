#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <unordered_map>
#include <stack>
#include <algorithm>
#include <windows.h> // Windows API のために追加
// ==============================
// UTF-8 次の 1 文字のバイト数
// ==============================
size_t utf8_next_char_size(const std::string& s, size_t i)
{
    unsigned char c = s[i];

    if ((c & 0x80) == 0x00) return 1; // 0xxxxxxx
    if ((c & 0xE0) == 0xC0) return 2; // 110xxxxx
    if ((c & 0xF0) == 0xE0) return 3; // 1110xxxx
    if ((c & 0xF8) == 0xF0) return 4; // 11110xxx

    return 1; // 不正 → 1バイトとして処理
}

// ==============================
// トークン化
// ==============================
std::vector<std::string> tokenize(
    const std::string& code,
    const std::vector<std::string>& insts)
{
    std::vector<std::string> tokens;

    for (size_t i = 0; i < code.size(); ) {
        bool matched = false;

        for (const auto& inst : insts) {

            if (i + inst.size() <= code.size() &&
                code.compare(i, inst.size(), inst) == 0)
            {
                tokens.push_back(inst);

#ifdef _DEBUG
                std::cout << "MATCH: \"" << inst
                    << "\" at " << i << "\n";
#endif

                i += inst.size();
                matched = true;
                break;
            }
        }

        if (!matched) {
            size_t skip = utf8_next_char_size(code, i);

#ifdef _DEBUG
            std::cout << "SKIP: \""
                << code.substr(i, skip)
                << "\" bytes=" << skip
                << " at " << i << "\n";
#endif
            i += skip;
        }
    }
    return tokens;
}

// ==============================
// [LOVE] と [Placer] のジャンプ
// ==============================
std::unordered_map<int, int> buildJumpTable(const std::vector<std::string>& tokens)
{
    std::unordered_map<int, int> jump;
    std::stack<int> st;

    for (int i = 0; i < tokens.size(); i++) {
        if (tokens[i] == "LOVE") {
            st.push(i);
        }
        else if (tokens[i] == "Placer") {
            if (st.empty()) {
                throw std::runtime_error("対応する「LOVE」がありません");
            }
            int j = st.top();
            st.pop();
            jump[j] = i;
            jump[i] = j;
        }
    }

    if (!st.empty()) {
        throw std::runtime_error("「Placer」が不足しています");
    }

    return jump;
}

// ==============================
// 命令は長い順にソート
// ==============================
std::vector<std::string> sortInstructions(std::vector<std::string> insts)
{
    std::sort(insts.begin(), insts.end(),
        [](const std::string& a, const std::string& b) {
            return a.size() > b.size();
        });
    return insts;
}

// ==============================
// メイン
// ==============================
int main(int argc, char* argv[])
{
    // --- 【修正箇所】 ---
#ifdef _WIN32
    // Windows API を使ってコンソールコードページをUTF-8に設定
    // std::system("chcp 65001"); は不要になります。
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif

    if (argc < 2) {
        std::cerr << "使い方: ./DestroyFxxk <code.df>\n";
        return 1;
    }

    // UTF-8 を壊さないようにバイナリで読む
    std::string path = argv[1];
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "ファイルを開けません: " << path << "\n";
        return 1;
    }

    // 命令一覧
    std::vector<std::string> insts = {
        "Destroy",
        "Board",
        "DESTROY!!!",
        "U",
        ":flag_de:",
        "198cm120kg",
        "LOVE",
        "Placer"
    };
    insts = sortInstructions(insts);

    // ファイル全部読む
    std::string code(
        (std::istreambuf_iterator<char>(file)),
        std::istreambuf_iterator<char>()
    );
    file.close();

    // トークン化
    auto tokens = tokenize(code, insts);
    auto jump = buildJumpTable(tokens);

#ifdef _DEBUG
    std::cout << "=== Tokens ===\n";
    for (int i = 0; i < tokens.size(); i++) {
        std::cout << i << ": [" << tokens[i] << "]\n";
    }
    std::cout << "==============\n";
#endif

    // ============================================
    // 実行
    // ============================================
    int ip = 0;
    std::vector<int> tape(30000, 0);
    int ptr = 0;

    while (ip < tokens.size()) {

        const auto& inst = tokens[ip];

        if (inst == "DESTROY!!!") {
            tape[ptr] = (tape[ptr] + 1) & 0xFF;
        }
        else if (inst == "U") {
            tape[ptr] = (tape[ptr] - 1) & 0xFF;
        }
        else if (inst == "Destroy") {
            ptr = (ptr + 1) % tape.size();
        }
        else if (inst == "Board") {
            ptr = (ptr == 0 ? tape.size() - 1 : ptr - 1);
        }
        else if (inst == ":flag_de:") {
            std::cout << (char)tape[ptr];
        }
        else if (inst == "198cm120kg") {
            int c = std::cin.get();
            tape[ptr] = (c == EOF ? 0 : c);
        }
        else if (inst == "LOVE") {
            if (tape[ptr] == 0) {
                ip = jump[ip];
                continue;
            }
        }
        else if (inst == "Placer") {
            if (tape[ptr] != 0) {
                ip = jump[ip];
                continue;
            }
        }

        ip++;
    }

    return 0;
}
