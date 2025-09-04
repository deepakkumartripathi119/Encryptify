# Encryptify

How Argon2id Works Internally 🔑
You can think of Argon2id as a three-stage, memory-intensive obstacle course for a password. Its primary goal is to make brute-force guessing extremely slow and expensive for an attacker.

Memory Allocation and Initialization: First, Argon2id allocates a large block of memory (the "memory cost" parameter you set). It then fills this entire block with pseudo-random values derived from the password and salt. This step is designed to make the algorithm memory-hard, forcing attackers to use a lot of RAM, which is expensive to get in large quantities for parallel attacks (like on GPUs).

Iterative Passes: Argon2id then makes several passes over this memory block (the "time cost" parameter). In each pass, it reads from different parts of the memory and combines the data with a hashing function. It uses a hybrid approach (Argon2id) where the first pass is done randomly to prevent side-channel attacks, and subsequent passes are done deterministically to make it resistant to time-memory trade-off attacks.

Final Hash: After all the passes are complete, the entire memory block is compressed down into a single, fixed-size output. This final hash is the secure key that your project uses for encryption.

Argon2id is more than just repeated hashing; it's a carefully designed process that deliberately consumes both CPU time and memory, making it the current gold standard for password hashing.

How ChaCha20-Poly1305 Works Internally 🔒
ChaCha20-Poly1305 is a modern, fast, and secure authenticated stream cipher. Its core purpose is to encrypt a continuous stream of data efficiently and verify that the data hasn't been tampered with. It does this by combining two separate algorithms:

ChaCha20 (The "ChaCha" Part): This is the core stream cipher. It takes your derived key and a unique number called a nonce (or a salt in our case) and generates a long stream of pseudo-random bytes. This is called a keystream. To encrypt your data, it simply XORs this keystream with your compressed file data, just like your original cipher, but with a much more secure and unpredictable keystream.

Poly1305 (The "Poly" Part): This is the authentication algorithm that ensures data integrity. As ChaCha20 encrypts the data, Poly1305 simultaneously calculates a small, unique tag (the "authentication tag") based on the key and the encrypted data. This tag is like a digital signature for your encrypted file.

During decryption, ChaCha20 decrypts the data, and Poly1305 recalculates its authentication tag. It then compares this new tag with the tag stored in the file. If they don't match, the decryption fails, and you know the file has been corrupted or tampered with. This is why it's called an authenticated stream cipher.