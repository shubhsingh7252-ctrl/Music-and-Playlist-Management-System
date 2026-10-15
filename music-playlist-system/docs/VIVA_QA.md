# Viva — expected questions & answers

**1. Project C mein kyun?** DSA subject ka project hai, isliye backend poora C mein: HTTP server (sockets), JSON, auth aur saare data structures khud likhe — koi external library nahi.

**2. Hash table kyun aur collision kaise handle kiya?** Username/ID/token se O(1) lookup. Separate chaining (har bucket mein linked list). Load factor 0.75 cross hone par table double hoti hai (rehash).

**3. Playlist ke liye doubly linked list kyun, array kyun nahi?** Reorder/remove mein elements shift nahi karne padte; node ke `prev/next` pointers badalte hain. Array mein O(n) shifting hoti.

**4. AVL tree kyun, normal BST kyun nahi?** Sorted data insert karne par BST ek line (linked list) ban jata hai, O(n). AVL rotations (LL, RR, LR, RL) se height ≤ 1.44·log n rakhta hai. Test mein 1000 sorted inserts ke baad height ≤ 12.

**5. Trie ka kya fayda?** Prefix search O(L), jahan L = typed letters; songs ki sankhya se independent.

**6. Queue aur Stack kahan?** Queue = play queue (enqueue back, cursor next/prev). Stack = recently played; ⏮ dabane par pop karke pichla gaana milta hai. Stack bounded hai (30), full hone par sabse purana hat jata hai.

**7. Heap ka use?** Admin ke top-5 most played, aur recommendations ko score ke hisaab se rank karne mein. Saare n songs heap mein daalke sirf top k pop karte hain: O(n + k log n), poora sort (O(n log n)) nahi karna padta.

**8. Recommendation kaise kaam karti hai?** Songs graph ke nodes hain. Edge weight = same artist 3 + same genre 2 + same mood 1. BFS depth 2 tak; direct neighbours ka score ×10, 2-hop ka ×1; max-heap se top-k.

**9. Sorting algorithm?** Merge sort — stable aur worst-case O(n log n). Artist sort mein comparator pehle artist, phir title dekhta hai.

**10. Password safe kaise?** Plain password store nahi hota. Random salt + SHA-256 ko 3000 baar chala ke hash save hota hai; compare constant-time mein. (Production mein bcrypt/Argon2 behtar hai — future scope.)

**11. Multiple users ek saath kaise?** Har connection ke liye alag thread; saari shared data structures ek global mutex (`store_lock`) se protected hain.

**12. Data kahan save hota hai?** `data/musicms.db` (tab-separated text) — har change par atomically (temp file + rename) likha jata hai, startup par memory ke data structures mein load. PPT ka MySQL = future scope (schema `docs/schema.sql` mein).

**13. Limitations?** Sessions memory mein hain (server restart par dobara login), HTTP (HTTPS nahi), text-file DB bade scale ke liye nahi, search linear scan hai (suggestions trie se).
