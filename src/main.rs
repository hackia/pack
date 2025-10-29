use std::path::PathBuf;

use clap::{Arg, ArgMatches, Command};

fn cli() -> ArgMatches {
    Command::new("k")
        .version(env!("CARGO_PKG_VERSION"))
        .about("A tool for packing files")
        .subcommand(
            Command::new("unpack")
                .about("Unpack files")
                .arg(
                    Arg::new("archive")
                        .short('a')
                        .long("archive")
                        .help("the archives to unpack")
                        .required(true),
                )
                .arg(
                    Arg::new("destination")
                        .short('d')
                        .long("destination")
                        .help("the destination directory")
                        .required(true),
                ),
        )
        .subcommand(
            Command::new("pack")
                .about("Pack files")
                .arg(
                    Arg::new("input")
                        .long("if")
                        .require_equals(true)
                        .help("the directory to pack")
                        .required(true),
                )
                .arg(
                    Arg::new("output")
                        .long("of")
                        .require_equals(true)
                        .help("The destination file")
                        .required(true),
                ),
        )
        .get_matches()
}

#[must_use]
pub fn files(directory: &str) -> Vec<PathBuf> {
    let mut files: Vec<PathBuf> = Vec::new();
    let mut w = ignore::WalkBuilder::new(directory);
    w.add_custom_ignore_filename(".packignore");
    w.same_file_system(true);
    let walker: ignore::Walk = w.build();
    for result in walker.flatten() {
        let path = result.path();
        if path.is_file() {
            files.push(path.to_path_buf());
        }
    }
    files
}
fn main() {
    let matches: ArgMatches = cli();

    if matches.subcommand_matches("pack").is_some() {
        if let Some(sub_m) = matches.subcommand_matches("pack") {
            let input = sub_m.get_one::<String>("input").unwrap();
            let output = sub_m.get_one::<String>("output").unwrap();
            let files_to_pack = files(input);
            println!("Files to pack from '{input}': {files_to_pack:?}");
            println!("Output will be saved to '{output}'");
            println!("Packing files...");
        }
        // Handle pack command
    } else if matches.subcommand_matches("unpack").is_some() {
        if let Some(sub_m) = matches.subcommand_matches("unpack") {
            let archive = sub_m.get_one::<String>("archive").unwrap();
            let destination = sub_m.get_one::<String>("destination").unwrap();
            println!("Unpacking archive '{archive}'");
            println!("Files will be extracted to '{destination}'");
            println!("Unpacking files...");
        }
    } else {
        eprintln!("No valid subcommand was used.");
    }
}
