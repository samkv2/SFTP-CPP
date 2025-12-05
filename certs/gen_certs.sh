#!/bin/bash
mkdir -p keys

# Generate CA Key and Cert
openssl genrsa -out keys/ca.key 2048
openssl req -x509 -new -nodes -key keys/ca.key -sha256 -days 1024 -out keys/ca.crt -subj "/C=US/ST=State/L=City/O=MyOrg/OU=MyUnit/CN=MyCA"

# Generate Server Key and CSR
openssl genrsa -out keys/server.key 2048
openssl req -new -key keys/server.key -out keys/server.csr -subj "/C=US/ST=State/L=City/O=MyOrg/OU=MyUnit/CN=localhost"

# Sign Server Cert with CA
openssl x509 -req -in keys/server.csr -CA keys/ca.crt -CAkey keys/ca.key -CAcreateserial -out keys/server.crt -days 500 -sha256

# Generate Client Key and CSR
openssl genrsa -out keys/client.key 2048
openssl req -new -key keys/client.key -out keys/client.csr -subj "/C=US/ST=State/L=City/O=MyOrg/OU=MyUnit/CN=client"

# Sign Client Cert with CA
openssl x509 -req -in keys/client.csr -CA keys/ca.crt -CAkey keys/ca.key -CAcreateserial -out keys/client.crt -days 500 -sha256

echo "Certificates generated in certs/keys/"
