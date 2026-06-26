use std::env;

use memshadow::protocol::MemshadowHeader;

fn hex_to_bytes(s: &str) -> Result<Vec<u8>, String> {
    if s.len() % 2 != 0 {
        return Err("hex length must be even".to_string());
    }
    let mut out = Vec::with_capacity(s.len() / 2);
    for i in (0..s.len()).step_by(2) {
        let byte = u8::from_str_radix(&s[i..i + 2], 16).map_err(|e| e.to_string())?;
        out.push(byte);
    }
    Ok(out)
}

fn main() {
    let args: Vec<String> = env::args().collect();
    if args.len() != 3 && args.len() != 4 {
        eprintln!("usage: conformance_probe <header|message> <hex> [keyhex]");
        std::process::exit(2);
    }

    let kind = &args[1];
    let data = match hex_to_bytes(&args[2]) {
        Ok(data) => data,
        Err(err) => {
            eprintln!("{err}");
            std::process::exit(2);
        }
    };
    let key = if args.len() == 4 {
        match hex_to_bytes(&args[3]) {
            Ok(k) => Some(k),
            Err(err) => {
                eprintln!("{err}");
                std::process::exit(2);
            }
        }
    } else {
        None
    };

    let header = match MemshadowHeader::unpack(&data[..32.min(data.len())]) {
        Some(h) => h,
        None => {
            eprintln!("unpack failed");
            std::process::exit(1);
        }
    };

    match kind.as_str() {
        "header" => {
            let repacked = header.pack();
            println!(
                "{{\"kind\":\"header\",\"magic\":{},\"version\":{},\"priority\":{},\"msg_type\":{},\"flags\":{},\"batch_count\":{},\"raw_flags_batch\":{},\"payload_len\":{},\"sequence_num\":{},\"repacked_hex\":\"{}\"}}",
                header.magic,
                header.version,
                header.priority,
                header.msg_type,
                header.flags_batch & 0x00FF,
                (header.flags_batch >> 8) & 0x00FF,
                header.flags_batch,
                header.payload_len,
                header.sequence_num,
                hex::encode(repacked)
            );
        }
        "message" => {
            let (header, payload) = match MemshadowHeader::unpack_message(&data, key.as_deref()) {
                Ok(v) => v,
                Err(err) => {
                    eprintln!("{err}");
                    std::process::exit(1);
                }
            };
            let repacked = {
                let mut h = header.clone();
                h.pack_message(&payload, key.as_deref())
            };
            print!(
                "{{\"kind\":\"message\",\"version\":{},\"priority\":{},\"msg_type\":{},\"flags\":{},\"batch_count\":{},\"raw_flags_batch\":{},\"payload_len\":{},\"sequence_num\":{},\"payload_hex\":\"",
                header.version,
                header.priority,
                header.msg_type,
                header.flags_batch & 0x00FF,
                (header.flags_batch >> 8) & 0x00FF,
                header.flags_batch,
                payload.len(),
                header.sequence_num
            );
            for b in &payload {
                print!("{:02x}", b);
            }
            println!("\",\"repacked_hex\":\"{}\"}}", hex::encode(repacked));
        }
        _ => {
            eprintln!("unknown kind");
            std::process::exit(2);
        }
    }
}
