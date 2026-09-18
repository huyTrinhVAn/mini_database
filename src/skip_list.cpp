#include "skip_list.hpp"

#include <random>

// TODO: implement SkipListNode:: and SkipList:: member functions here
SkipListNode::SkipListNode(const std::string &key, const std::string &value, int level)
{
    this->key = key;
    this->value = value;
    forward = std::vector<SkipListNode *>(level, nullptr);
}
SkipList::SkipList()
{
    head_ = new SkipListNode("", "", kMaxLevel);
    level_ = 0;
}

SkipList::~SkipList()
{
    // forward[0] (Level 1 / tầng đáy) chứa MỌI node trong list, nên chỉ cần
    // duyệt đúng tầng này là đi qua đủ toàn bộ node, không cần đụng các tầng khác.
    SkipListNode *current = head_;
    while (current != nullptr)
    {
        // Phải lưu lại "next" TRƯỚC khi delete current -- sau delete, current
        // đã là bộ nhớ không hợp lệ, đọc current->forward[0] lúc đó là use-after-free.
        SkipListNode *next = current->forward[0];
        delete current;
        current = next;
    }
}

int SkipList::random_level()
{
    static std::random_device rd;
    static unsigned int seed = rd();
    static std::mt19937 engine(seed);
    static std::bernoulli_distribution coin_flip(0.5);

    int level = 1;
    while (coin_flip(engine) && level < kMaxLevel)
    {
        level += 1;
    }
    return level;
}

void SkipList::put(const std::string &key, const std::string &value)
{
    // update[i] sẽ lưu "node đứng ngay trước vị trí cần chèn" ở tầng i.
    // Khởi tạo mặc định = head_ để xử lý luôn case new_level vượt qua level_ hiện tại
    // (những tầng "mới mở" thì node đứng trước luôn là head_, vì trước đó chưa từng có ai ở tầng đó).
    std::vector<SkipListNode *> update(kMaxLevel, head_);

    // current dùng để "đi dò đường" qua các tầng, bắt đầu từ head_ (node sentinel, không chứa dữ liệu thật)
    SkipListNode *current = head_;

    // Duyệt từ tầng cao nhất đang có xuống tầng 0.
    // Đi từ tầng cao giúp "nhảy cóc" qua nhiều node cùng lúc -> đây là lý do skip list nhanh (O(log n)).
    for (int i = level_ - 1; i >= 0; i -= 1)
    {
        // Ở tầng i, đi tới khi nào node tiếp theo có key < key cần chèn thì dừng
        // (current->forward[i] != nullptr phải check trước để tránh dereference null)
        while (current->forward[i] != nullptr && current->forward[i]->key < key)
        {
            current = current->forward[i];
        }
        // current hiện đang là node cuối cùng ở tầng i mà key vẫn nhỏ hơn key cần chèn
        // -> chính là "điểm neo" để nối node mới vào sau này
        update[i] = current;

        // LƯU Ý: khi vòng for chuyển sang tầng thấp hơn (i giảm), current KHÔNG bị reset về head_
        // -> tận dụng lại vị trí đã đi được ở tầng cao, không cần dò lại từ đầu
    }

    // Bước qua node kế tiếp ở tầng 0 để kiểm tra xem key đã tồn tại trong list chưa
    current = current->forward[0];

    // Trường hợp 1: key đã tồn tại -> chỉ cần cập nhật value, không cần tạo node mới
    if (current != nullptr && current->key == key)
    {
        current->value = value;
        return;
    }

    // Trường hợp 2: key chưa tồn tại -> cần tạo node mới và chèn vào list
    // random_level() quyết định node mới sẽ "cao" bao nhiêu tầng (theo cơ chế tung đồng xu 50/50)
    int new_level = random_level();

    // Nếu node mới random ra level cao hơn cả level_ hiện tại của cả list,
    // cần "mở" thêm các tầng mới -> ở những tầng đó, node đứng trước chắc chắn là head_
    // (vì trước đó chưa có node nào từng vươn tới tầng cao như vậy)
    if (new_level > level_)
    {
        for (int i = level_; i < new_level; i++)
        {
            update[i] = head_;
        }
        level_ = new_level; // cập nhật lại chiều cao hiện tại của skip list
    }

    // Tạo node mới, forward của nó có đúng new_level phần tử (chỉ tồn tại ở new_level tầng đầu tiên)
    SkipListNode *new_node = new SkipListNode(key, value, new_level);

    // Nối node mới vào từng tầng từ 0 đến new_level - 1
    for (int i = 0; i < new_level; i++)
    {
        // Bước 1: node mới "mượn lại" đích cũ mà update[i] đang trỏ tới
        // (để không làm mất phần list phía sau nó)
        new_node->forward[i] = update[i]->forward[i];

        // Bước 2: update[i] đổi hướng, trỏ sang node mới
        // -> thứ tự 2 bước này bắt buộc, đảo ngược sẽ làm node mới tự trỏ vào chính nó (mất phần đuôi list)
        update[i]->forward[i] = new_node;
    }
}

bool SkipList::get(const std::string &key, std::string &out_value)
{
    // current dùng để "đi dò đường" qua các tầng, bắt đầu từ head_ (node sentinel, không chứa dữ liệu thật)
    SkipListNode *current = head_;
    // Duyệt từ tầng cao nhất đang có xuống tầng 0.
    // Đi từ tầng cao giúp "nhảy cóc" qua nhiều node cùng lúc -> đây là lý do skip list nhanh (O(log n)).
    for (int i = level_ - 1; i >= 0; i -= 1)
    {
        // Ở tầng i, đi tới khi nào node tiếp theo có key < key cần chèn thì dừng
        // (current->forward[i] != nullptr phải check trước để tránh dereference null)
        while (current->forward[i] != nullptr && current->forward[i]->key < key)
        {
            current = current->forward[i];
        }
        // current hiện đang là node cuối cùng ở tầng i mà key vẫn nhỏ hơn key cần chèn
        // -> chính là "điểm neo" để nối node mới vào sau này
    }
    current = current->forward[0];
    if (current != nullptr && current->key == key)
    {
        out_value = current->value;
        return true;
    }
    else
    {
        return false;
    }
}