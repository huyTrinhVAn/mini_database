# 1.1 — Hash Table (Separate Chaining)

## Why start here
The hash table is the heart of a KV store. Before tackling an LSM Tree or distribution,
you need to understand why average lookup is O(1), where that breaks down (worst case), and
the tradeoffs between collision-handling strategies — these same tradeoffs resurface when you
design consistent hashing in Stage 3.

## Concepts to understand before coding
Research these on your own (Wikipedia, CLRS, any course) and don't start coding until you can
answer the question attached to each:

1. **Hash function** — turns a key (string, int, ...) into an integer.
   - Question: what properties make a hash function "good"? Why do many real systems
     (Redis, Cassandra) write their own hash function (FNV-1a, MurmurHash, xxHash) instead of
     relying on the language's built-in one?
1. Một hash function "tốt" cần những tính chất gì?Không giống như mật mã học (cryptographic hashing như SHA-256, bcrypt) vốn ưu tiên tính bảo mật và chống đảo ngược, hash function trong systems engineering ưu tiên tốc độ và độ phân phối:Uniform Distribution (Phân phối đều): Các giá trị băm ra phải rải đều ngẫu nhiên trên toàn bộ dải giá trị số nguyên ($0$ đến $2^{32}-1$ hoặc $2^{64}-1$). Nếu dữ liệu đầu vào lệch, kết quả băm vẫn phải phẳng, tránh việc dồn cục vào một số ít bucket hoặc node.Avalanche Effect (Hiệu ứng tuyết lở): Thay đổi dù chỉ 1 bit ở input (ví dụ đổi "user_1" thành "user_2") phải khiến xấp xỉ 50% các bit ở output bị lật ngẫu nhiên.High Throughput & Low Latency: Cực nhanh trên CPU. Tiêu tốn ít chu kỳ lệnh (cycles per byte), hạn chế tối đa rẽ nhánh logic (branching), tận dụng tốt cache L1/L2 và các tập lệnh phần cứng (SIMD, vectorization).Cross-platform Determinism (Tính tiền định đa nền tảng): Cùng một chuỗi byte đầu vào, dù chạy trên x86_64, ARM64, hệ điều hành Linux hay Windows, kiến trúc Big-Endian hay Little-Endian, kết quả số nguyên sinh ra bắt buộc phải đồng nhất 100%.2. Vì sao Redis và Cassandra không dùng hash built-in của ngôn ngữ?Các hệ thống phân tán và database quy mô lớn bắt buộc phải dùng các thuật toán chuyên biệt như MurmurHash3 (Cassandra), SipHash / MurmurHash2 (Redis) hay xxHash vì các lý do sau:a. Tính nhất quán đa nền tảng và môi trường phân tán (Distributed Consistency)Vấn đề của built-in: Hàm băm mặc định của các ngôn ngữ (như Object.hashCode() trong Java hay std::hash trong C++) chỉ cam kết đúng trong cùng một process hoặc một runtime cụ thể. Chúng không có tiêu chuẩn chung xuyên suốt các ngôn ngữ.Yêu cầu hệ thống: Trong Cassandra, cluster gồm nhiều node viết bằng Java, nhưng client gửi request có thể viết bằng Go, Python, C# hay Rust. Nếu dựa vào hashCode() của Java, client viết bằng Go sẽ không thể tự tính toán token để biết partition key nằm ở node nào. Việc dùng chuẩn chung như MurmurHash3 giúp client và server ở mọi ngôn ngữ luôn tính ra cùng một kết quả.b. Dữ liệu lưu xuống đĩa hoặc tái khởi động (Data Persistence & Restart)Nhiều ngôn ngữ hiện đại (như Python 3, Go, Rust) mặc định chèn một giá trị ngẫu nhiên bí mật (random seed) vào hàm băm mỗi lần process khởi động lại (nhằm chống tấn công HashDoS).Nếu Redis hay Cassandra dùng hash của runtime, mỗi khi khởi động lại node:Mọi partition mapping, ring token trong Consistent Hashing sẽ bị xáo trộn hoàn toàn.Dữ liệu lưu trên đĩa (SSTable, RDB/AOF) sẽ trỏ sai partition và không thể truy xuất lại được.c. Hiệu năng tính toán cấp độ phần cứngCác hàm built-in cổ điển thường xử lý dữ liệu theo kiểu duyệt từng byte một qua vòng lặp.Các thuật toán như xxHash hoặc MurmurHash được thiết kế để đọc và trộn dữ liệu theo khối lớn (4 bytes, 8 bytes hoặc 16 bytes cùng lúc bằng thanh ghi 64/128-bit). Chúng đạt tốc độ từ vài GB/s đến hơn 10 GB/s trên một luồng CPU, tiệm cận tốc độ đọc tuần tự của RAM, giúp Redis xử lý hàng trăm nghìn RPS mà không bị nghẽn ở bước băm key.d. Tránh bẫy dồn cục (Poor Avalanche) của thư viện chuẩnVí dụ: java.lang.String.hashCode() dùng công thức cơ bản:$$s[0]\cdot 31^{n-1} + s[1]\cdot 31^{n-2} + \dots + s[n-1]$$Công thức nhân tuyến tính này có khả năng trộn bit rất kém đối với các key có định dạng tương tự nhau (ví dụ: chuỗi ID tăng dần order_0001, order_0002, các UUID có cùng prefix/timestamp). Nếu dùng công thức này để chọn partition node, dữ liệu sẽ bị hotspotting (dồn cục vào một vài node cụ thể), phá vỡ nguyên lý cân bằng tải của hệ thống phân tán.
2. **Collision** — two different keys hash to the same bucket.
   - Question: can collisions be avoided entirely? Why or why not?
   Về mặt lý thuyết và toán học tổng quát: Không thể tránh hoàn toàn đụng độ (collision).Lý do cốt lõi bắt nguồn từ hai nguyên lý:Nguyên lý chuồng bồ câu (Pigeonhole Principle): Không gian đầu vào (domain - tập hợp các string, object, struct bất kỳ) là vô hạn hoặc lớn hơn rất nhiều so với không gian đầu ra (range - thường là số nguyên 32-bit hoặc 64-bit). Ví dụ:Số lượng chuỗi ký tự độ dài tối đa 20 ký tự vượt xa con số $2^{64}$.Khi ánh xạ một tập hợp lớn hơn $N$ phần tử vào $M$ ô chứa (với $N > M$), chắc chắn tồn tại ít nhất một ô chứa từ 2 phần tử trở lên.Modulo Reduction (Ánh xạ vào Bucket): Ngay cả khi giá trị hash integer không trùng nhau, bước đưa giá trị đó vào mảng bucket qua phép tính index = hash % bucket_count sẽ thu hẹp không gian giá trị xuống còn vài chục hay vài nghìn bucket, khiến xác suất đụng độ tăng vọt theo Nghịch lý ngày sinh (Birthday Paradox).Ngoại lệ duy nhất: Perfect Hashing (Băm hoàn hảo)Đụng độ có thể tránh hoàn toàn 100% nếu và chỉ nếu thỏa mãn điều kiện:Tập hợp key là tĩnh và biết trước toàn bộ (Static & Known Keys): Ví dụ như danh sách các từ khóa dành riêng của một ngôn ngữ lập trình (if, else, return, class...) khi viết compiler, hoặc bảng từ vựng tĩnh trong từ điển.Khi đó, có thể dùng các thuật toán như CHM hoặc các công cụ sinh mã như GNU gperf để sinh ra một hàm Minimal Perfect Hash Function (MPHF):Ánh xạ chính xác $N$ key đã biết vào đúng $N$ bucket riêng biệt.Đạt được truy xuất $O(1)$ tuyệt đối trong worst-case mà không có bất kỳ đụng độ nào.Tuy nhiên, trong các hệ thống thực tế (database, cache, hash map thông thường) nơi dữ liệu liên tục được thêm mới/cập nhật động, collision là điều không thể tránh khỏi, và hệ thống bắt buộc phải sử dụng các chiến lược giải quyết đụng độ (Separate Chaining, Open Addressing, Cuckoo Hashing...).

3. **Separate chaining vs open addressing** — the two main collision-handling strategies.
   - Question: what structure does chaining use inside each bucket (linked list vs vector), and
     how does that affect cache locality? Where does open addressing (linear/quadratic probing,
     Robin Hood hashing) get tricky when deleting an element (hint: tombstones)?
     1. Cấu trúc bên trong bucket của Chaining (Linked List vs Vector) và ảnh hưởng tới Cache LocalityLinked List:Cách thức: Mỗi phần tử là một node độc lập được cấp phát động trên Heap, nối với nhau bằng con trỏ (next/prev).Ảnh hưởng Cache Locality: Rất kém. Các node nằm rải rác ở các vùng nhớ khác nhau. Khi xảy ra đụng độ và phải duyệt danh sách, CPU liên tục phải "nhảy theo con trỏ" (pointer chasing), làm vô hiệu hóa CPU prefetcher và gây ra L1/L2 Cache Misses liên tục. Đồng thời, nó tốn thêm bộ nhớ để lưu con trỏ (8–16 bytes/node).Vector (Dynamic Array):Cách thức: Các phần tử rơi vào cùng bucket được xếp liền kề nhau trong một mảng phẳng.Ảnh hưởng Cache Locality: Vượt trội. Do các phần tử nằm trên dải ô nhớ liên tục, khi CPU đọc phần tử đầu tiên, toàn bộ khối Cache Line (thường là 64 bytes) sẽ được nạp sẵn vào cache, giúp việc quét qua các phần tử kế tiếp gần như tức thì.Đánh đổi: Tốn chi phí reallocate và copy mảng khi kích thước tăng, kèm theo chi phí giữ struct vector ở các bucket rỗng.2. Điểm hóc búa khi xóa trong Open Addressing (Tombstone)Trong Open Addressing, toàn bộ dữ liệu nằm trên một mảng phẳng duy nhất. Khi tìm kiếm một key, thuật toán sẽ đi theo chuỗi dò tìm (probe sequence) cho đến khi:Gặp đúng key cần tìm $\rightarrow$ Thành công.Gặp một ô hoàn toàn trống (EMPTY) $\rightarrow$ Dừng lại và kết luận key không tồn tại.Cái bẫy khi xóa ngây thơ (Naive Delete): Nếu chỉ đơn giản xóa dữ liệu và trả ô đó về trạng thái EMPTY, bạn sẽ làm đứt chuỗi dò tìm (broken probe sequence). Các phần tử bị đụng độ trước đó vốn phải dạt sang các ô phía sau sẽ không bao giờ được tìm thấy nữa, vì thuật toán gặp ô EMPTY này và dừng lại sớm.Giải pháp Tombstone: Để khắc phục, ô bị xóa phải được gắn một cờ đặc biệt gọi là Tombstone (hoặc DELETED).Khi Search: Gặp Tombstone thì bắt buộc đi tiếp (không được dừng).Khi Insert: Được phép ghi đè vào ô mang cờ Tombstone để tái sử dụng chỗ trống.Vấn đề hiệu năng (Tombstone Accumulation): Nếu hệ thống có tần suất xóa/chèn liên tục, bảng sẽ bị lấp đầy bởi Tombstone. Thao tác tìm kiếm một key không tồn tại sẽ không thể dừng sớm mà phải duyệt qua hàng loạt ô Tombstone, khiến thời gian truy xuất từ $O(1)$ bị suy biến về $O(N)$.
 
5. **Rehashing** — once load factor crosses the threshold, grow the bucket count and move every
   element over.
   - Question: what's the amortized cost of rehashing across many inserts, and why (relate this
     to the amortized analysis of a dynamic array/vector)?
Mặc dù một lần resize đơn lẻ tốn thời gian $O(N)$ (vì phải cấp phát mảng mới lớn gấp đôi, tính lại hash index và chuyển toàn bộ $N$ phần tử sang mảng mới), nhưng chi phí trung bình trên mỗi lần chèn qua một chuỗi dài các thao tác vẫn chỉ là $O(1)$.Mối liên hệ với phân tích Amortized của Dynamic Array (Vector)Cơ chế rehashing của Hash Table hoạt động tương tự như việc nhân đôi dung lượng (capacity * 2) trong std::vector của C++ hay ArrayList của Java:Tần suất giảm dần theo hàm mũ: Để kích hoạt lần rehashing tiếp theo, số lượng phần tử cần thêm vào phải tăng gấp đôi so với lần trước. Các đợt tốn kém $O(N)$ xảy ra ngày càng thưa thớt (tại các mốc kích thước $1, 2, 4, 8, 16, \dots, N$).Tổng chi phí của toàn bộ các lần resize:Giả sử bắt đầu từ mảng kích thước ban đầu và chèn liên tục đến khi có $N$ phần tử:$$\text{Tổng chi phí dọn nhà} = 1 + 2 + 4 + 8 + \dots + \frac{N}{2} + N = \sum_{i=0}^{\log_2 N} 2^i = 2N - 1 < 2N$$Phân bổ chi phí (Amortized Analysis):Tổng chi phí cho $N$ lần chèn = (Chi phí ghi $N$ phần tử bình thường) + (Chi phí di dời của các đợt rehashing).$\text{Tổng chi phí} = N \times O(1) + 2N = 3N$.Chia đều cho $N$ lần chèn:$$\text{Amortized Cost} = \frac{3N}{N} = 3 \implies O(1)$$Góc nhìn trực quan: Phương pháp Kế toán (Accounting Method)Hãy coi mỗi thao tác insert là một lần trả phí bằng "đồng xu":Chi phí thực tế để đặt 1 phần tử vào ô nhớ chỉ tốn 1 đồng.Hệ thống thu của bạn 3 đồng:1 đồng trả ngay cho việc chèn chính nó vào mảng hiện tại.2 đồng còn lại được gửi tiết kiệm vào "tài khoản ngân hàng" của chính phần tử đó.Khi bảng băm đầy và phải nhân đôi kích thước từ $N$ lên $2N$:$N/2$ phần tử mới thêm vào gần nhất đã tích lũy được: $\frac{N}{2} \times 2 = N$ đồng tiết kiệm.Số tiền này vừa đủ để chi trả cho việc di dời toàn bộ $N$ phần tử sang mảng mới mà không cần thu thêm bất kỳ đồng nào.Nhờ đó, mọi lần chèn đều được bảo đảm có chi phí danh nghĩa ổn định là $O(1)$.
## Functional spec
Design and implement a hash table yourself (a C++ template is the natural way to generalize over
key/value types) supporting:

- `put(key, value)` — insert, or overwrite if the key already exists
- `get(key)` — return the value if present, or signal "not found" (hint: `std::optional`)
- `erase(key)` — remove a key, return whether it was actually removed
- `contains(key)` — existence check
- `size()` — current element count
- Automatic rehashing (growing bucket count) once load factor crosses a threshold you choose

## Non-functional requirements
- O(1) average complexity for put/get/erase — you should be able to explain why your design
  achieves that when it's reviewed.
- Don't use `std::unordered_map` (that's the built-in hash table — the point is to write your own).
- Using `std::hash<Key>` as your hash function is fine to start; writing your own (try FNV-1a for
  strings) is encouraged to understand how a hash function actually works.

## Tests to write before moving on
No test framework needed yet (gtest etc.) — manual asserts in `main` or a separate test file are
enough at this stage. At minimum cover:
1. Insert then get returns the correct value.
2. Overwriting an existing key — `size()` doesn't grow, and the new value is returned.
3. Erase a key, then `contains()` is false.
4. Erase a key that doesn't exist — no crash, returns false.
5. Insert enough elements to trigger a rehash — verify every element is still retrievable and
   correct afterward.

## Self-check before moving to 1.2 (Skip List / B-Tree)
Be able to answer these (write it out; if you're fumbling, it means you haven't internalized it yet):
- Why does a hash table give fast lookups but poor support for range queries (e.g. "give me every
  key from A to M")? This is exactly why LSM Trees / B-Trees exist — you'll see this clearly in
  the next stage.
  Hash table tra cứu nhanh (O(1)) vì hash function ánh xạ trực tiếp key → vị trí, không cần so sánh với các key khác. Nhưng chính hash function đó lại cố tình xáo trộn thứ tự của key để tránh va chạm (collision) — nên 2 key gần nhau về giá trị (như "Apple" và "Apricot") có thể rơi vào 2 bucket cách xa nhau bất kỳ. Vì không còn giữ được thứ tự, hash table không thể "nhảy" tới đúng khoảng cần tìm — phải duyệt qua toàn bộ (O(n)) để lọc ra key nằm trong range. Ngược lại, B-Tree/LSM Tree giữ nguyên thứ tự sắp xếp của key trong cấu trúc của nó, nên có thể trả lời range query hiệu quả (O(log n) để tìm điểm bắt đầu, rồi duyệt tuần tự).
- If `bucket_count` is a prime number instead of a power of two, what's the benefit/cost? (relate
  this to distribution quality when the hash function isn't perfectly random)
  Câu hỏi này đi vào chi tiết khá sâu về cách chọn số lượng bucket — một chủ đề kinh điển khi thiết kế hash table. Để mình phân tích rõ.

## Vấn đề gốc: `%` với số bucket

Nhớ lại công thức tính index:
```cpp
index = hash_value % buckets_.size();
```

Kết quả của `%` phụ thuộc rất nhiều vào việc `buckets_.size()` là **số nguyên tố** hay **lũy thừa của 2** (power of two).

## Trường hợp: `bucket_count` là lũy thừa của 2 (ví dụ 16, 32, 64...)

Khi số chia là lũy thừa của 2, phép `% 2^k` **tương đương về mặt toán học** với việc **chỉ lấy k bit thấp nhất** của `hash_value`:

```
hash_value % 16   ==   lấy 4 bit thấp nhất của hash_value
```

**Vấn đề nảy sinh:** nếu hàm hash **không hoàn toàn ngẫu nhiên** ở các bit thấp (nhiều hash function thực tế có xu hướng "yếu" — kém ngẫu nhiên — ở vài bit thấp nhất, dù các bit cao vẫn phân bố tốt), thì việc chỉ dựa vào k bit thấp sẽ khiến **nhiều key khác nhau vô tình rơi vào cùng 1 bucket** — dù hash value của chúng nhìn tổng thể khá khác nhau.

Ví dụ đơn giản hóa: giả sử hash function có pattern nào đó khiến bit thấp nhất luôn có xu hướng lặp lại theo 1 quy luật nhẹ (không hoàn toàn ngẫu nhiên) — thì khi `% 16` (chỉ nhìn 4 bit thấp), bạn "khuếch đại" điểm yếu đó lên, khiến phân bố vào bucket bị lệch (skewed), nhiều bucket trống trong khi vài bucket khác quá tải.

## Trường hợp: `bucket_count` là số nguyên tố (ví dụ 17, 31, 61...)

Khi số chia là **số nguyên tố**, phép `%` sẽ **buộc phải "trộn" toàn bộ các bit** của `hash_value` để tính ra kết quả — không có cách nào rút gọn phép tính `% p` (p là số nguyên tố) thành việc chỉ nhìn 1 nhóm bit cụ thể như trường hợp lũy thừa của 2.

→ **Lợi ích:** vì số nguyên tố không "cộng hưởng" (không chia hết) với bất kỳ pattern chu kỳ nào ẩn trong hash value theo lũy thừa 2, nên `% p` có xu hướng **phân bố đều hơn** vào các bucket — **ngay cả khi hàm hash không hoàn hảo** (không hoàn toàn ngẫu nhiên).

Đây chính là lý do nhiều implementation hash table cổ điển (ví dụ `java.util.Hashtable` bản cũ, hoặc nhiều sách giáo trình) khuyên dùng **số nguyên tố** làm `bucket_count`.

## Vậy tại sao nhiều hash table hiện đại (như `std::unordered_map`, hoặc nhiều implementation khác) lại dùng **lũy thừa của 2**?

Vì lũy thừa của 2 có **lợi ích về tốc độ**:

```cpp
hash_value % 16        // phép chia (%), có thể chậm hơn 1 chút trên 1 số CPU
hash_value & 15         // dùng bitwise AND (&) — thường NHANH HƠN đáng kể so với %
```

Khi `bucket_count` là lũy thừa của 2, phép `% N` **có thể thay bằng `& (N-1)`** — phép AND ở mức bit, vốn cực nhanh trên phần cứng (thường chỉ 1 cycle CPU, trong khi `%` với số bất kỳ có thể tốn nhiều cycle hơn, tùy CPU).

→ Nhưng đánh đổi lại: nếu hash function không đủ tốt (yếu ở bit thấp), bucket_count dạng lũy thừa 2 dễ gây phân bố lệch hơn — như đã giải thích ở trên.

## Cách các hash table hiện đại giải quyết mâu thuẫn này

Nhiều implementation (bao gồm `std::unordered_map` phổ biến) chọn **dùng lũy thừa của 2** (để tận dụng tốc độ `&`), nhưng **bù lại bằng cách "trộn" (mix) thêm hash value trước khi lấy modulo** — ví dụ dùng kỹ thuật gọi là "hash finalizer" hoặc "avalanche mixing" (như trong MurmurHash, xxHash...) để đảm bảo **mọi bit** của hash value đều tốt/ngẫu nhiên, không chỉ bit thấp — từ đó xóa bỏ nhược điểm của phương pháp lũy thừa 2 mà vẫn giữ được tốc độ.

## Tóm tắt (trả lời ngắn gọn)

| | Số nguyên tố | Lũy thừa của 2 |
|---|---|---|
| **Lợi ích** | Phân bố đều hơn, "bù đắp" cho hash function không hoàn hảo (vì buộc trộn toàn bộ bit) | Tốc độ nhanh hơn — `%` có thể thay bằng `&` (bitwise AND) |
| **Chi phí** | Phép `%` với số nguyên tố bất kỳ thường **chậm hơn** phép AND | Nếu hash function yếu ở bit thấp → dễ bị phân bố lệch, nhiều va chạm hơn dự kiến |
| **Dùng khi nào** | Khi không tin tưởng hoàn toàn vào chất lượng hash function, ưu tiên phân bố đều | Khi hash function đã được thiết kế tốt (trộn đều mọi bit) hoặc có bước "mix" bổ sung, ưu tiên tốc độ |
- Resizing (rehashing) blocks all operations while data is being moved — for a real KV store
  serving live traffic, how serious is this? (this is a preview of the "incremental rehashing"
  problem that Redis actually solves)
  Trong bài tập HashTable đơn giản của bạn, resize gây "dừng mọi thứ" không sao cả vì bạn không phục vụ nhiều client đồng thời. Nhưng khi đây là hạ tầng production thực tế (như Redis), 1 lần resize chặn toàn bộ hệ thống trong vài trăm mili giây tới vài giây có thể gây hậu quả nghiêm trọng (timeout hàng loạt, cascading failure) — đây chính là động lực khiến Redis phát triển kỹ thuật incremental rehashing: rải nhỏ công việc resize ra theo thời gian, thay vì làm 1 lần rồi chặn hết mọi request trong lúc đó.
  fact thêm
  Đây chính là vấn đề Redis giải quyết: Incremental Rehashing

Thay vì resize "một phát ăn ngay" (di chuyển hết trong 1 lần, chặn mọi thứ), Redis dùng kỹ thuật gọi là incremental rehashing — ý tưởng cốt lõi:

Giữ 2 bảng hash cùng lúc: bảng cũ (old_table) và bảng mới (new_table, đã tăng kích thước).
Thay vì di chuyển hết ngay, Redis di chuyển từng bucket nhỏ một, rải ra qua nhiều lần gọi lệnh (mỗi khi có 1 lệnh GET/SET đến, Redis "tiện tay" di chuyển thêm 1 chút dữ liệu từ bảng cũ sang bảng mới trước khi xử lý lệnh đó).
Trong lúc này, cả 2 bảng đều được kiểm tra khi tra cứu (tìm ở new_table trước, không thấy thì tìm ở old_table) — nên hệ thống vẫn phục vụ được request bình thường, không có khoảng "đứng hình" nào cả — chỉ là quá trình resize kéo dài hơn 1 chút (dàn trải theo thời gian) để đổi lấy việc không bao giờ chặn traffic.


## Project setup (do it yourself)
No starter code here — decide the directory layout, build system (CMake or just calling `g++`
directly), and file names yourself. Light suggestion: separate the interface (header) from the
test/demo code so it's easier to reuse once you build the layers on top of it, but that's your call.

When you're done, you can ask Claude to review your code (not write it for you) or ask if you get
stuck on a specific concept.
# thêm

Load factor	Tốc độ	Bộ nhớ
Rất thấp (0.1)	Cực nhanh (hầu như 0 collision)	Lãng phí — cấp phát rất nhiều bucket trống, không dùng tới
~0.7 - 1.0	Nhanh (chấp nhận vài collision nhỏ)	Cân bằng — không lãng phí quá nhiều
Quá cao (>2-3)	Chậm dần (nhiều collision)	Tiết kiệm bộ nhớ nhưng đánh đổi tốc độ