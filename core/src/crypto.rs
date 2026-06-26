// Pure Rust Native SHA-256 and HMAC-SHA256 Implementation
// Zero external dependencies.

const K: [u32; 64] = [
    0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
    0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
    0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
    0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
    0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
    0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
    0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
    0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2,
];

pub fn sha256(data: &[u8]) -> [u8; 32] {
    let mut h = [
        0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a,
        0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19,
    ];

    let bit_len = (data.len() as u64) * 8;
    let mut padded = data.to_vec();
    padded.push(0x80);

    while (padded.len() % 64) != 56 {
        padded.push(0);
    }

    padded.extend_from_slice(&bit_len.to_be_bytes());

    for chunk in padded.chunks_exact(64) {
        let mut w = [0u32; 64];
        for (i, byte_chunk) in chunk.chunks_exact(4).enumerate() {
            w[i] = u32::from_be_bytes(byte_chunk.try_into().unwrap());
        }

        for i in 16..64 {
            let s0 = w[i - 15].rotate_right(7) ^ w[i - 15].rotate_right(18) ^ (w[i - 15] >> 3);
            let s1 = w[i - 2].rotate_right(17) ^ w[i - 2].rotate_right(19) ^ (w[i - 2] >> 10);
            w[i] = w[i - 16].wrapping_add(s0).wrapping_add(w[i - 7]).wrapping_add(s1);
        }

        let mut a: u32 = h[0];
        let mut b: u32 = h[1];
        let mut c: u32 = h[2];
        let mut d: u32 = h[3];
        let mut e: u32 = h[4];
        let mut f: u32 = h[5];
        let mut g: u32 = h[6];
        let mut k_val: u32 = h[7];

        for i in 0..64 {
            let s1 = e.rotate_right(6) ^ e.rotate_right(11) ^ e.rotate_right(25);
            let ch = (e & f) ^ (!e & g);
            let temp1 = k_val.wrapping_add(s1).wrapping_add(ch).wrapping_add(K[i]).wrapping_add(w[i]);
            let s0 = a.rotate_right(2) ^ a.rotate_right(13) ^ a.rotate_right(22);
            let maj = (a & b) ^ (a & c) ^ (b & c);
            let temp2 = s0.wrapping_add(maj);

            k_val = g;
            g = f;
            f = e;
            e = d.wrapping_add(temp1);
            d = c;
            c = b;
            b = a;
            a = temp1.wrapping_add(temp2);
        }

        h[0] = h[0].wrapping_add(a);
        h[1] = h[1].wrapping_add(b);
        h[2] = h[2].wrapping_add(c);
        h[3] = h[3].wrapping_add(d);
        h[4] = h[4].wrapping_add(e);
        h[5] = h[5].wrapping_add(f);
        h[6] = h[6].wrapping_add(g);
        h[7] = h[7].wrapping_add(k_val);
    }

    let mut out = [0u8; 32];
    for (i, val) in h.iter().enumerate() {
        out[i * 4..(i + 1) * 4].copy_from_slice(&((*val as u32).to_be_bytes()));
    }
    out
}

pub fn hmac_sha256(key: &[u8], message: &[u8]) -> [u8; 32] {
    let mut actual_key = [0u8; 64];
    if key.len() > 64 {
        let hashed_key = sha256(key);
        actual_key[..32].copy_from_slice(&hashed_key);
    } else {
        actual_key[..key.len()].copy_from_slice(key);
    }

    let mut o_pad = [0x5c; 64];
    let mut i_pad = [0x36; 64];
    for i in 0..64 {
        o_pad[i] ^= actual_key[i];
        i_pad[i] ^= actual_key[i];
    }

    let mut inner_data = i_pad.to_vec();
    inner_data.extend_from_slice(message);
    let inner_hash = sha256(&inner_data);

    let mut outer_data = o_pad.to_vec();
    outer_data.extend_from_slice(&inner_hash);
    sha256(&outer_data)
}

macro_rules! quarter_round {
    ($a:expr, $b:expr, $c:expr, $d:expr) => {{
        $a = $a.wrapping_add($b); $d ^= $a; $d = $d.rotate_left(16);
        $c = $c.wrapping_add($d); $b ^= $c; $b = $b.rotate_left(12);
        $a = $a.wrapping_add($b); $d ^= $a; $d = $d.rotate_left(8);
        $c = $c.wrapping_add($d); $b ^= $c; $b = $b.rotate_left(7);
    }};
}

pub fn chacha20_block(key: &[u8; 32], nonce: &[u8; 12], counter: u32, block: &mut [u8; 64]) {
    let mut state = [0u32; 16];
    state[0] = 0x61707865;
    state[1] = 0x3320646e;
    state[2] = 0x79622d32;
    state[3] = 0x6b206574;
    for i in 0..8 {
        state[4 + i] = u32::from_le_bytes(key[i * 4..(i + 1) * 4].try_into().unwrap());
    }
    state[12] = counter;
    for i in 0..3 {
        state[13 + i] = u32::from_le_bytes(nonce[i * 4..(i + 1) * 4].try_into().unwrap());
    }

    let mut working_state = state;
    for _ in 0..10 {
        quarter_round!(working_state[0], working_state[4], working_state[8], working_state[12]);
        quarter_round!(working_state[1], working_state[5], working_state[9], working_state[13]);
        quarter_round!(working_state[2], working_state[6], working_state[10], working_state[14]);
        quarter_round!(working_state[3], working_state[7], working_state[11], working_state[15]);

        quarter_round!(working_state[0], working_state[5], working_state[10], working_state[15]);
        quarter_round!(working_state[1], working_state[6], working_state[11], working_state[12]);
        quarter_round!(working_state[2], working_state[7], working_state[8], working_state[13]);
        quarter_round!(working_state[3], working_state[4], working_state[9], working_state[14]);
    }

    for i in 0..16 {
        working_state[i] = working_state[i].wrapping_add(state[i]);
        block[i * 4..(i + 1) * 4].copy_from_slice(&working_state[i].to_le_bytes());
    }
}

pub fn chacha20_encrypt(key: &[u8; 32], nonce: &[u8; 12], initial_counter: u32, data: &mut [u8]) {
    let mut block = [0u8; 64];
    let mut counter = initial_counter;
    for chunk in data.chunks_mut(64) {
        chacha20_block(key, nonce, counter, &mut block);
        for i in 0..chunk.len() {
            chunk[i] ^= block[i];
        }
        counter = counter.wrapping_add(1);
    }
}
