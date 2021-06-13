target/debug/server-x86_64-unknown-linux-gnu: native/server-x86_64-unknown-linux-gnu
	cp native/server-x86_64-unknown-linux-gnu target/debug/server-x86_64-unknown-linux-gnu

target/debug/native: native/
	cp -r native target/debug/native

.PHONY: dev build

dev: target/debug/native target/debug/server-x86_64-unknown-linux-gnu
	RUST_LOG=trace yarn tauri dev
build:
	yarn tauri build
