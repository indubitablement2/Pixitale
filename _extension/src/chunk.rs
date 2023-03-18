use super::*;

// Compile to ~15 ops.
pub unsafe fn chunk_set_active(x: i64, y: i64) {
    let idx = (y >> 5) + (x >> 5) * CHUNK_WIDTH;
    let bitmask = (1 << ((x & 31) + 32)) | (1 << (y & 31));
    let chunk = CHUNKS.offset(idx as isize);
    chunk.write(chunk.read() | bitmask);
}

/// Return if this row is active.
pub fn chunk_is_row_inactive(chunk: i64, local_y: i64) -> bool {
    chunk & (1 << local_y) == 0
}

/// Return the active rows and columns (y_start..y_end, x_start..x_end).
pub fn chunk_active_rect(chunk: i64) -> (std::ops::Range<i64>, std::ops::Range<i64>) {
    if chunk == 0 {
        return (0..0, 0..0);
    }

    let cols = (chunk >> 32) as u32;
    let rows = chunk as u32;

    let col_start = cols.trailing_zeros() as i64;
    let col_end = 32 - cols.leading_zeros() as i64;
    let row_start = rows.trailing_zeros() as i64;
    let row_end = 32 - rows.leading_zeros() as i64;

    (row_start..row_end, col_start..col_end)
}

#[test]
fn test_chunk() {
    unsafe { _test_chunk() }
}
unsafe fn _test_chunk() {
    new_empty_grid(0, 0);
    assert_eq!(CHUNKS.read(), 0);
    assert_eq!(CHUNKS.add(1).read(), 0);
    assert_eq!(chunk_active_rect(CHUNKS.read()), (0..0, 0..0));

    for x in 0..32 {
        for y in 0..32 {
            CHUNKS.write(0);
            chunk_set_active(x, y);

            assert!(CHUNKS.read() != 0);
            assert!(!chunk_is_row_inactive(CHUNKS.read(), y));
            assert_eq!(
                chunk_active_rect(CHUNKS.read()),
                (y..y + 1, x..x + 1),
                "x: {}, y: {}",
                x,
                y
            );

            chunk_set_active(x, y + 32);
        }
    }

    assert_eq!(CHUNKS.add(1).read().count_ones(), i64::BITS as u32);
    assert_eq!(chunk_active_rect(CHUNKS.add(1).read()), (0..32, 0..32));

    assert_eq!(CHUNKS.add(2).read(), 0);
    chunk_set_active(0, 64);
    chunk_set_active(31, 64 + 31);
    assert_eq!(chunk_active_rect(CHUNKS.add(2).read()), (0..32, 0..32));
    for local_y in 1..31 {
        assert!(chunk_is_row_inactive(CHUNKS.add(2).read(), local_y));
    }

    use rand::prelude::*;
    for _ in 0..100 {
        CHUNKS.add(3).write(0);
        for _ in 0..32 {
            chunk_set_active(
                thread_rng().gen_range(0..32),
                96 + thread_rng().gen_range(0..32),
            );
        }
        let (rows, _columns) = chunk_active_rect(CHUNKS.add(3).read());
        for local_y in 0..32 {
            if !chunk_is_row_inactive(CHUNKS.add(3).read(), local_y) {
                assert!(rows.contains(&local_y));
            }
        }
    }

    print_chunk(CHUNKS.add(3).read());
}

pub fn print_chunk(chunk: i64) {
    use colored::*;

    let (rows, columns) = chunk_active_rect(chunk);
    println!("rows: {:?}, columns: {:?}", rows, columns);
    let mut skip_rect = 0;
    let mut skip_row = 0;

    for y in 0..32 {
        for x in 0..32 {
            if !rows.contains(&y) || !columns.contains(&x) {
                print!("{}", ".".green());
                skip_rect += 1;
            } else if chunk_is_row_inactive(chunk, y) {
                print!("{}", ".".yellow());
                skip_row += 1;
            } else {
                print!("{}", "X".red());
            }
        }
        println!();
    }
    let skip_rect_percent = skip_rect as f32 / (32.0 * 32.0) * 100.0;
    let skip_row_percent = skip_row as f32 / (32.0 * 32.0) * 100.0;
    let skip_percent = skip_rect_percent + skip_row_percent;
    println!(
        "skiped by rect: {}({:.2}%), skipped by bitfield: {}({:.2}%), {}/1024({:.2}%) skipped",
        skip_rect,
        skip_rect_percent,
        skip_row,
        skip_row_percent,
        skip_rect + skip_row,
        skip_percent,
    );
}
