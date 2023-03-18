use super::*;
use std::ops::Range;
struct Hasher {
    hasher: RandomState,
}
impl Hasher {
    fn from_seed(seed: u64) -> Self {
        Self {
            hasher: RandomState::with_seed(seed as usize),
        }
    }

    fn hash(&self, key: u32) -> usize {
        self.hasher.hash_one(key as u64) as usize
    }
}

pub struct PhfRange {
    hasher: Hasher,
    table: Vec<u64>,
}
impl PhfRange {
    pub fn new(values: &[(u32, Range<usize>)], max_iter: usize) -> Self {
        let min_len = values.iter().filter(|(_, v)| !v.is_empty()).count() + 1;
        let mut table_len = min_len.next_power_of_two();

        let mut table: Vec<Option<Range<usize>>> = vec![None; table_len];

        let mut seeder = thread_rng();
        let mut hasher: Hasher;

        let mut total_iter = 0;
        let mut iter = 0;
        'out: loop {
            if iter > max_iter {
                iter = 0;
                table_len = (table_len + 1).next_power_of_two();
                table = vec![None; table_len];
                continue;
            }
            table.fill(None);
            iter += 1;
            total_iter += 1;

            hasher = Hasher::from_seed(seeder.gen());

            for (k, v) in values.iter() {
                let idx = hasher.hash(*k) % table_len;

                if let Some(range) = &table[idx] {
                    if !range_eq(v, range) {
                        continue 'out;
                    }
                } else {
                    table[idx] = Some(v.clone());
                }
            }

            break;
        }

        println!("Finished in {} iterations", total_iter);
        println!("Table length: {}, min lenght: {}", table_len, min_len);

        let table = table
            .into_iter()
            .map(|v| {
                if let Some(range) = v {
                    if range.is_empty() {
                        0
                    } else {
                        (range.start as u64) | ((range.end as u64) << 32)
                    }
                } else {
                    0
                }
            })
            .collect();

        Self { hasher, table }
    }

    pub fn get(&self, key: u32) -> Range<usize> {
        let idx = self.hasher.hash(key) % self.table.len();
        if let Some(range) = self.table.get(idx) {
            let start = (range & 0xFFFF_FFFF) as usize;
            let end = (range >> 32) as usize;

            start..end
        } else {
            return 0..0;
        }
    }
}

fn range_eq(a: &Range<usize>, b: &Range<usize>) -> bool {
    if a.is_empty() && b.is_empty() {
        true
    } else if a == b {
        true
    } else {
        false
    }
}

fn gen_expected(num: u32, min: usize, max: usize, chance_empty: f64) -> Vec<(u32, Range<usize>)> {
    let mut rng = thread_rng();
    (0u32..num)
        .map(|i| {
            let start = rng.gen_range(min..max);
            let range = if rng.gen_bool(chance_empty) {
                start..start
            } else {
                start..start + rng.gen_range(min..max)
            };

            (i, range)
        })
        .collect()
}

#[test]
fn test_phf_hash() {
    const NUM: u32 = 300;
    const CHANCE_EMPTY: f64 = 0.5;

    let expected = gen_expected(NUM, 0, NUM as usize, CHANCE_EMPTY);

    let phf_range = PhfRange::new(&expected, 10000);

    for (k, expected) in expected.iter() {
        let result = phf_range.get(*k);
        assert!(range_eq(expected, &result));
    }
}

#[test]
fn sum() {
    const NUM: i32 = 1000;
    let mut sum = 0;
    for i in 0..NUM {
        for _ in i..NUM {
            sum += 1;
        }
    }

    println!("{} -> {}B", sum, sum * 8);
}

#[bench]
fn bench_phf_hash(b: &mut test::Bencher) {
    let expected = gen_expected(1000, 0, 1000 as usize, 0.5);
    let phf_range = PhfRange::new(&expected, 10000);

    b.iter(|| {
        for (k, _) in expected.iter() {
            test::black_box(phf_range.get(*k));
        }
    });
}

#[bench]
fn bench_ahash(b: &mut test::Bencher) {
    let expected = gen_expected(1000, 0, 1000 as usize, 0.5);
    let ahash = AHashMap::from_iter(expected.iter().cloned());

    b.iter(|| {
        for (k, _) in expected.iter() {
            test::black_box(ahash.get(k).unwrap());
        }
    });
}

#[bench]
fn bench_btree_map(b: &mut test::Bencher) {
    let expected = gen_expected(1000, 0, 1000 as usize, 0.5);
    let ahash = std::collections::BTreeMap::from_iter(expected.iter().cloned());

    b.iter(|| {
        for (k, _) in expected.iter() {
            test::black_box(ahash.get(k).unwrap());
        }
    });
}

#[bench]
fn bench_vec(b: &mut test::Bencher) {
    let expected = gen_expected(1000, 0, 1000 as usize, 0.5);
    let mut vec = vec![0..0; expected.len()];
    for (k, v) in expected.iter() {
        vec[*k as usize] = v.clone();
    }
    b.iter(|| {
        for (k, _) in expected.iter() {
            test::black_box(vec.get(*k as usize).unwrap());
        }
    });
}
