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

fn main() {
    let matches: ArgMatches = cli();

    if matches.subcommand_matches("unpack").is_some() {
        println!("Unpacking files...");
        // Handle unpack command
    } else {
        println!("Packing files...");
        // Handle pack command
    }
}
