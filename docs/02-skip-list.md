# 1.2a — Skip List

## Why this comes next
The hash table gives O(1) average lookup but can't answer "give me every key between A and M"
without scanning everything — you already worked this out yourself in the Stage 1.1 self-check.
A skip list keeps keys in sorted order while still giving expected O(log n) search/insert/delete,
so it answers exactly the query a hash table can't. It's also not a coincidence that this is next:
Stage 1.4's LSM Tree needs an in-memory sorted structure (the "memtable"), and real systems
(LevelDB, RocksDB) use a skip list there for reasons you'll rediscover in the self-check below —
what you build here gets reused almost directly.

## Concepts to understand before coding
1. **The core idea — "express lanes" over a sorted linked list.** A plain sorted linked list gives
   correct order but O(n) search (no way to skip ahead). A skip list adds extra linked lists on top
   — each higher level skips over more nodes, like express lanes above a local-stops lane. Searching
   starts at the top (sparsest) level and drops down a level whenever the next node would overshoot.
   - Question: sketch (on paper) a skip list with values 1, 3, 5, 7, 9, 11 across 3 levels, and trace
     the search path for `7`. Where does the search actually save comparisons compared to scanning
     the bottom-level list one node at a time?
     Sketch (mô tả cấu trúc 3 tầng):

Level 3:  1 ------------------------> 9
Level 2:  1 -----------> 5 --------> 9
Level 1:  1 -> 3 -> 5 -> 7 -> 9 -> 11
Level 1 (dưới cùng): chứa đầy đủ tất cả giá trị, sắp xếp tăng dần.
Level 2: chỉ giữ một tập con thưa hơn — 1, 5, 9.
Level 3 (trên cùng, thưa nhất): chỉ còn 1, 9.
Các giá trị xuất hiện ở nhiều tầng (1, 5, 9) được nối với nhau bằng con trỏ dọc xuống tầng dưới.

Trace tìm kiếm giá trị 7:

Bắt đầu ở level 3, tại node 1. Node kế tiếp là 9, mà 9 > 7 → không thể đi tiếp ở tầng này, tụt xuống level 2 (vẫn ở node 1).
Ở level 2, từ 1, node kế tiếp là 5. Vì 5 < 7 → di chuyển sang 5.
Vẫn ở level 2, từ 5, node kế tiếp là 9, mà 9 > 7 → tụt xuống level 1 (vẫn ở node 5).
Ở level 1, từ 5, node kế tiếp là 7 → khớp, tìm thấy.

Đường đi: 1(L3) → 1(L2) → 5(L2) → 5(L1) → 7(L1).

Chỗ tiết kiệm so sánh:

Nếu quét tuần tự chỉ ở level 1, bạn phải đi qua 1 → 3 → 5 → 7, tức là bắt buộc phải ghé qua node 3. Nhưng với skip list, node 3 không bao giờ được chạm tới — vì "làn nhanh" ở level 2 nhảy thẳng từ 1 sang 5, bỏ qua nó hoàn toàn. Đó chính là chỗ tiết kiệm: các tầng cao hơn giúp bỏ qua từng cụm node ở tầng dưới thay vì phải duyệt từng node một, và mức tiết kiệm này càng rõ rệt khi danh sách càng lớn (O(log n) thay vì O(n)).
2. **Probabilistic balancing.** Unlike a balanced BST (AVL, red-black) which maintains balance via
   explicit rotations after every insert/delete, a skip list decides each new node's height randomly
   (a coin-flip process) at insertion time and never rebalances anything afterward.
   - Question: if each node's height is decided by flipping a coin and adding a level for every
     "heads" until you get "tails" (capped at some max), what's the probability a given node reaches
     height ≥ k? What does that imply about how many nodes exist at each level as you go up?
     Xác suất node đạt độ cao ≥ k:
     P(height≥k)=(1 / 2)^ (k - 1)
     Giải thích ngắn gọn: mọi node mặc định có tầng 1. Để leo lên thêm mỗi tầng, phải tung đồng xu ra "ngửa" (xác suất 1/2); ra "sấp" thì dừng. Để đạt được tầng thứ k, cần (k−1) lần ngửa liên tiếp, nên xác suất là (1/2)^(k−1).
     Ngụ ý về số node mỗi tầng:

Với n node tổng cộng, số node kỳ vọng ở tầng k là:
E[số node tầng k] = n / 2^(k-1)
→ Tầng 1 có ~n node, tầng 2 có ~n/2, tầng 3 có ~n/4, ... — giảm theo cấp số nhân khi lên cao. Số tầng cao nhất kỳ vọng rơi vào khoảng log₂(n), vì đó là điểm mà n/2^(k-1) giảm xuống còn khoảng 1.

So với AVL/red-black tree: thay vì cân bằng chủ động bằng rotation sau mỗi thao tác, skip list dùng tính ngẫu nhiên để "tự động" tạo ra cấu trúc gần giống cây cân bằng — số tầng ~ O(log n), phân bố node mỗi tầng giảm dần đều — mà không cần logic rebalancing tường minh nào.
3. **Why expected O(log n), not worst-case O(log n).** This is the key tradeoff versus a balanced
   BST.
   - Question: what's the (extremely unlikely but technically possible) worst case for a skip list's
     search time? Why is a balanced BST's O(log n) a stronger guarantee than a skip list's expected
     O(log n)? Is that difference something you'd actually worry about in practice — why or why not?
Worst case của skip list:

Vì chiều cao mỗi node được quyết định bằng tung đồng xu ngẫu nhiên, về mặt lý thuyết không có gì ngăn tất cả các lần tung đều ra "sấp" ngay từ đầu — tức là mọi node đều chỉ có height = 1, không có tầng nào cao hơn tầng 1 cả. Khi đó skip list suy biến thành một linked list thường, và tìm kiếm trở thành O(n) — tệ nhất có thể.

(Một biến thể worst case khác: tất cả node "ngẫu nhiên" đều leo lên rất cao và tập trung dồn về một phía, khiến các tầng trên không phân bố đều, làm mất tác dụng "nhảy cóc". Nhưng trường hợp suy biến về O(n) là dễ hình dung nhất.)

Tại sao O(log n) của balanced BST là bảo đảm mạnh hơn:

AVL / red-black tree: cân bằng được duy trì bằng logic tường minh (rotation) sau mỗi insert/delete. Cây không bao giờ có thể lệch quá một giới hạn cố định (ví dụ chiều cao 2 nhánh con lệch nhau tối đa 1 với AVL) — đây là một invariant được chứng minh toán học và enforce bằng code, đúng 100% các lần, không có ngoại lệ. Nên O(log n) ở đây là worst-case guarantee.
Skip list: O(log n) chỉ là kỳ vọng (expected), dựa trên phân phối xác suất của việc tung đồng xu qua nhiều lần insert. Không có cơ chế nào chủ động ngăn chặn trường hợp xui — nó chỉ cực kỳ khó xảy ra về mặt xác suất (giống việc tung đồng xu ra sấp liên tục hàng nghìn lần), chứ không phải bị cấm.

Về bản chất: BST cân bằng đảm bảo bằng cấu trúc, còn skip list đảm bảo bằng xác suất.

Có đáng lo trong thực tế không? → Không, hầu như không.

Lý do:

Xác suất worst case xảy ra giảm theo cấp số nhân với n. Với n lớn (hàng nghìn, hàng triệu phần tử), xác suất để performance suy biến đáng kể là cực kỳ nhỏ — nhỏ đến mức không đáng lo hơn nhiều rủi ro khác trong hệ thống (ví dụ hash collision trong hash table cũng có worst case O(n) nhưng chẳng ai sợ nó cả, vì lý do tương tự).
Đây chính xác là lý do các hệ thống production nổi tiếng như Redis (sorted sets), LevelDB, Lucene vẫn chọn skip list thay vì balanced BST — vì skip list đơn giản hơn nhiều để cài đặt và maintain (không cần logic rotation phức tạp, dễ làm concurrent/lock-free hơn), trong khi hiệu năng thực tế gần như luôn đạt O(log n) đúng như kỳ vọng.
Nếu bạn đang làm hệ thống hard real-time (ví dụ điều khiển máy bay, thiết bị y tế) nơi mà worst-case tuyệt đối quan trọng hơn hiệu năng trung bình, thì lúc đó sự khác biệt "guaranteed vs expected" mới thực sự đáng cân nhắc — nhưng đây là trường hợp rất hiếm trong lập trình ứng dụng thông thường.
4. **No rotations, ever.** A skip list insert only allocates a new node and relinks pointers at the
   levels that node participates in; delete only unlinks. Compare this mentally to what an AVL tree
   has to do after an insert that unbalances it.
   - Question: this simplicity is exactly why LevelDB/RocksDB picked skip lists for their memtable,
     which is written to concurrently by multiple threads. Why would "no rebalancing" matter a lot
     more once multiple threads are inserting into the same structure at once? (You don't need to
     implement concurrent access yet — just reason about why it'd be harder for a self-balancing
     tree here.)
Vấn đề cốt lõi: rotation làm thay đổi cấu trúc lan tỏa (cascading), còn skip list insert thì cục bộ (localized).

Ở AVL tree khi insert gây mất cân bằng:

Một lần insert có thể kích hoạt rotation không chỉ tại node vừa thêm, mà lan lên nhiều tổ tiên phía trên (trong trường hợp xấu, lan tới tận root). Mỗi rotation phải:

Thay đổi con trỏ cha-con của nhiều node cùng lúc (không chỉ 1-2 con trỏ, mà thường là 3+ con trỏ liên quan: node xoay, con của nó, cha của nó).
Cập nhật lại chiều cao/balance-factor của toàn bộ đường đi từ node vừa insert lên tới root.

Với single thread, đây không phải vấn đề — cứ làm tuần tự. Nhưng với multiple thread insert đồng thời:

Phạm vi khóa (locking) rất rộng và khó đoán trước. Vì bạn không biết trước insert nào sẽ gây rotation lan tới đâu (có thể chỉ ảnh hưởng local, có thể lan tới tận root), để đảm bảo an toàn bạn gần như buộc phải khóa một phần lớn cây — thậm chí nhiều implementation đơn giản phải khóa toàn bộ cây. Điều này giết chết tính song song — nhiều thread muốn insert cùng lúc nhưng phải xếp hàng chờ nhau.
Race condition tinh vi hơn. Hai thread insert ở hai nhánh khác nhau vẫn có thể cùng kích hoạt rotation chạm tới một node tổ tiên chung (ví dụ cả hai đều lan lên tới root). Việc chứng minh đúng đắn (correctness) cho concurrent AVL/red-black tree — đảm bảo không có 2 thread nào đồng thời xoay cùng một vùng con trỏ theo cách phá vỡ cấu trúc — là bài toán học thuật khó, cần các kỹ thuật phức tạp (fine-grained locking, lock coupling, hoặc lock-free với CAS nhiều bước).

Ngược lại, skip list insert:

Chỉ cần: (a) random height cho node mới, (b) tìm vị trí insert ở từng tầng mà node đó tham gia, (c) relink một vài con trỏ next tại đúng các tầng đó — không đụng tới bất kỳ node nào khác ngoài các "hàng xóm" trực tiếp ở mỗi tầng.
Không có hiệu ứng lan tỏa lên "tổ tiên" — vì skip list không có khái niệm cha-con theo nghĩa cây, chỉ có các con trỏ ngang (next) độc lập theo từng tầng.
Vì phạm vi thay đổi luôn cục bộ và có thể xác định trước (chỉ vài node lân cận, tối đa bằng chiều cao của node mới), việc concurrent-safe insert dễ implement hơn nhiều: có thể dùng lock-free CAS trên từng con trỏ next riêng lẻ (giống atomic linked-list insert), hoặc khóa rất hẹp (chỉ khóa các node lân cận bị ảnh hưởng) mà không sợ conflict lan rộng ra toàn cấu trúc.

Tóm lại: AVL/red-black cần rebalancing có thể lan tỏa toàn cục → concurrent insert cần khóa rộng hoặc thuật toán lock-free cực kỳ phức tạp để chứng minh đúng. Skip list insert chỉ relink cục bộ → dễ làm concurrent (thậm chí lock-free) hơn nhiều, vì phạm vi ảnh hưởng của mỗi thao tác được giới hạn và dự đoán trước — đây chính xác là lý do LevelDB/RocksDB chọn skip list cho memtable, nơi nhiều writer thread ghi đồng thời.
## Functional spec
Design and implement a skip list yourself, supporting:

- `put(key, value)` — insert, or overwrite if the key already exists
- `get(key)` — return the value if present, or signal "not found"
- `erase(key)` — remove a key, return whether it was actually removed
- `contains(key)` — existence check
- `range(start, end)` — return every key-value pair with `start <= key < end`, **in sorted key
  order**. This is the operation a hash table structurally cannot support efficiently — it's the
  whole point of this stage, don't skip it.

Design decisions you need to make and be able to justify (not prescribed here on purpose):
- Max level / height cap, and the probability `p` used for the coin-flip (commonly `0.5` or
  `0.25`) — what tradeoff does `p` control?
- How you represent "no node here yet" at the start of each level (a common approach: a dummy
  head node that exists at every level).

## Non-functional requirements
- Expected O(log n) for `put`/`get`/`erase`.
- `range(start, end)` should cost roughly O(log n + k) where k is the number of matching results
  — you find the starting point in O(log n), then it's a straight walk along the bottom level.
- Keys must be comparable (e.g. `std::string`, using `<`) — this is fundamentally different from
  the hash table, which only needed equality (`==`).

## Tests to write before moving on
1. Insert then get returns the correct value.
2. Overwriting an existing key updates the value without creating a duplicate entry.
3. Erase a key, then `contains()` is false.
4. Erase a key that doesn't exist — no crash, returns false.
5. Insert keys in random order, then call `range()` covering all of them — verify the result comes
   back in sorted order, not insertion order.
6. `range(start, end)` with a narrow window — verify it returns exactly the keys inside
   `[start, end)` and nothing outside it.

## Self-check before moving to 1.2b (B-Tree) or 1.3 (WAL)
- Explain in your own words why a skip list's height is randomized instead of fixed, and roughly
  what the expected height is for n elements (tie this to your answer on question 2 above).
- What do you give up, concretely, by choosing a probabilistic structure (skip list) over a
  strictly balanced one (B-Tree, red-black tree)? What do you gain?
- Now that you've built one: why did LevelDB/RocksDB pick a skip list specifically for the
  memtable? Say it in terms of the actual access pattern (frequent concurrent inserts, need for
  sorted iteration) rather than just "it's simpler."

## Project setup (do it yourself)
Same principle as the hash table: decide the file layout yourself. A reasonable option is a new
header (e.g. `src/skip_list.hpp`) alongside `hash_table.hpp`, plus its own test file in `tests/`.
As before, no starter code here — ask for a review once you've written something, or ask if you
get stuck on a specific concept.
