![screenshot](sftp-1.png)

# Secure File Transfer Protocol (SFTP-CPP) Instructions

This project implements a secure file transfer system using C++ and OpenSSL. It features a specific custom protocol over TLS 1.3 with AES encryption.

## Prerequisites
- Linux Environment
- `g++` (supporting C++17) or `cmake`
- `openssl` installed (`libssl-dev`)

## Installation & Build

1.  **Navigate to the project directory:**
    ```bash
    cd /home/captain/Documents/AntiGravity/SFTP-CPP
    ```

2.  **Generate SSL Certificates:**
    You must generate the keys and certificates for the secure connection to work.
    ```bash
    cd certs
    bash gen_certs.sh
    cd ..
    ```

3.  **Compile the Project:**
    Running the following command will build both the Server and the Client.
    ```bash
    g++ -std=c++17 -I include src/server/main.cpp src/server/server.cpp src/common/ssl_wrapper.cpp src/common/utils.cpp -o sftp_server -lssl -lcrypto -lpthread

    g++ -std=c++17 -I include src/client/main.cpp src/client/client.cpp src/common/ssl_wrapper.cpp src/common/utils.cpp -o sftp_client -lssl -lcrypto -lpthread
    ```

## Usage

### 1. Start the Server
The server handles file storage and connections.
```bash
./sftp_server
```
*The server will create a `server_storage` directory automatically.*

### 2. Start the Client
Open a new terminal.
```bash
./sftp_client [ServerIP]
```
If running locally, you can just run:
```bash
./sftp_client
```

### 3. Using the Client
The client features an interactive menu:

*   **List Remote Files**: Shows files currently stored on the server.
*   **Upload File**: Enter the path to a local file (e.g., `./docs/myfile.txt`) to upload it securely. You will see a progress bar.
*   **Download File**: Enter the name of a file on the server to download it to your current directory.
*   **Exit**: Close the connection.

## Security Features
*   **TLS 1.3**: All communication is encrypted using modern TLS standards.
*   **AES Encryption**: Data privacy is ensured via the cipher suites negotiated by OpenSSL.
*   **Certificate Pinning**: The client uses the generated CA certificate to verify the server's identity.

## Troubleshooting
*   **Connection Failed**: Ensure the server is running and the certificates were generated correctly in `certs/keys/`.
*   **Permission Denied**: Ensure you have read/write permissions in the directory.
