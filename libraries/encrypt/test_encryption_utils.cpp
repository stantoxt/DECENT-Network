#include <decent/encrypt/encryptionutils.hpp>
#include <decent/encrypt/custodyutils.hpp>

#include <fc/exception/exception.hpp>
#include <fc/log/logger.hpp>
#include <graphene/chain/protocol/decent.hpp>
#include <fc/io/raw.hpp>
#include <cstdio>
#include <iostream>
#include <sstream>
#include <string>
#include <fc/thread/thread.hpp>
#include <stdio.h>


using namespace std;

decent::crypto::custody_utils c;


using decent::crypto::d_integer;
void test_aes(decent::crypto::aes_key k)
{
   decent::crypto::AES_encrypt_file("/tmp/test_file.txt","/tmp/test_file.out", k);
   decent::crypto::AES_decrypt_file("/tmp/test_file.out","/tmp/test_file.orig",k);

}

void test_el_gamal(decent::crypto::aes_key k)
{
   cout<<"Catchpoint 0 \n";
   d_integer pk1 = decent::crypto::generate_private_el_gamal_key();
   d_integer pk2 = decent::crypto::generate_private_el_gamal_key();
   d_integer pubk1 = decent::crypto::get_public_el_gamal_key(pk1);
   d_integer pubk2 = decent::crypto::get_public_el_gamal_key(pk2);

   cout <<"pk1 = " << pk1.to_string();
   cout <<"pk2 = " << pk2.to_string();
   cout<<"Catchpoint 1 \n";
   decent::crypto::point secret;
   secret.first = d_integer(10000);
   secret.second = d_integer(1000000000);

   cout<<"Catchpoint 2 \n";
   decent::crypto::ciphertext ct1, ct2;
   decent::crypto::el_gamal_encrypt(secret, pubk1, ct1);

   decent::crypto::point received_secret;
   decent::crypto::el_gamal_decrypt(ct1,pk1,received_secret);
   cout<<"Catchpoint 3 \n";

   cout <<"Secret is: "<<secret.first.to_string()<<" "<<secret.second.to_string();

   cout <<"\n";

   cout <<"recovered secret is "<<received_secret.first.to_string()<<" "<<received_secret.second.to_string() <<"\n";

   decent::crypto::delivery_proof proof(CryptoPP::Integer::One(),CryptoPP::Integer::One(),CryptoPP::Integer::One(),CryptoPP::Integer::One(),CryptoPP::Integer::One());
   cout<<"Catchpoint 4 \n";
   decent::crypto::encrypt_with_proof(received_secret, pk1, pubk2, ct1, ct2, proof);
   cout<<"Catchpoint 5 \n";
   decent::crypto::point received_secret2;
   decent::crypto::el_gamal_decrypt(ct2,pk2,received_secret2);

   cout <<"recovered secret is "<<received_secret.first.to_string()<<" "<<received_secret.second.to_string() <<"\n";

   for (int i=0; i<1; i++)
      bool ret_val = decent::crypto::verify_delivery_proof(proof, ct1,ct2,pubk1,pubk2);
   /*if(ret_val)
      cout<< "everything OK!\n";*/

}

void test_shamir(decent::crypto::d_integer secret)
{
   decent::crypto::shamir_secret ss(5,9,secret);
   decent::crypto::point x0 = ss.split[0];
   decent::crypto::point x1 = ss.split[1];
   decent::crypto::point x2 = ss.split[2];
   decent::crypto::point x3 = ss.split[3];
   decent::crypto::point x4 = ss.split[6];

   decent::crypto::shamir_secret rs(5,9);
   rs.add_point(x0);
   rs.add_point(x1);
   rs.add_point(x2);
   rs.add_point(x3);
   rs.add_point(x4);
   if(rs.resolvable())
      rs.calculate_secret();
   cout << "Original secret: "<< secret.to_string() <<"\nReconstructed_secret: "<<rs.secret.to_string() <<"\n";
}



void test_passed_op(graphene::chain::ready_to_publish_operation& op){
   std::vector<char> data = fc::raw::pack(op);
   idump((op));

   graphene::chain::ready_to_publish_operation tmp;
   fc::datastream<const char*> ds( data.data(), data.size() );
   fc::raw::unpack( ds, tmp );
   idump((tmp));
}

void test_passing_add_level_reference(graphene::chain::ready_to_publish_operation& op){
   std::shared_ptr<fc::thread> new_thread = std::make_shared<fc::thread>("p2p");
   new_thread->async([&](){ return test_passed_op(op);}).wait();

}


void test_move(){


   graphene::chain::ready_to_publish_operation op;
   op.space = 1000;
   decent::crypto::d_integer a = decent::crypto::d_integer::from_string("12132131.");
   op.pubKey = a;
   op.price_per_MByte = 1;
   idump((op));
   test_passing_add_level_reference(op);
}

void test_custody(){


   //pbc_param_t par;
   //pbc_param_init_a_gen( par, 320, 1024 );
   //pbc_param_out_str(stdout,par);

   decent::crypto::custody_data cd;
   decent::crypto::custody_proof proof;
   proof.seed.data[0]=21; proof.seed.data[1] =155; proof.seed.data[2] = 231; proof.seed.data[3] = 98; proof.seed.data[4] = 1;

   c.create_custody_data(boost::filesystem::path("/tmp/content.zip"),cd );
   std::cout <<"done creating custody data, "<<cd.n<<" signatures generated\n";

   c.create_proof_of_custody(boost::filesystem::path("/tmp/content.zip"), cd,proof);
 //  idump((proof.mus));

  // cout<<"\n\n";
  // fc::raw::pack(cout, mus);
   if(c.verify_by_miner(cd, proof))
      std::cout <<"Something wrong during verification...\n";
   else
      std::cout <<"Verify sucessful!\n";

}

void test_key_manipulation()
{
   d_integer initial_key(123456789);
   char* buffer = (char*)malloc(1000);
   initial_key.Encode((byte*)buffer, 1000);
   fc::sha512 key1;
   for(int i=0; i<8; i++) key1._hash[i]=buffer[i];

   decent::crypto::aes_key k;
   for (int i = 0; i < CryptoPP::AES::MAX_KEYLENGTH; i++)
      k.key_byte[i] = key1.data()[i];



}

void generate_params(){
   int rbits = 80;
   int qbits = 256;

   pbc_param_t par;
   pairing_t pairing;
   element_t generator;

   pbc_param_init_a_gen(par, rbits, qbits);
   pairing_init_pbc_param(pairing, par);
   element_init_G1(generator, pairing);
   element_random(generator);

   pbc_param_out_str(stdout, par);
   element_printf("generator: %B\n",generator);
   element_printf("size of compressed: %i\n", element_length_in_bytes_compressed(generator));
   element_printf("size of element: %i\n", element_length_in_bytes(generator));
   pbc_param_clear(par);

}

void test_generator(){
   pairing_t pairing;
   pairing_init_set_str(pairing, _DECENT_PAIRING_PARAM_);

   mpz_t d1, d2, d3, d4, d5, d6, d7;
   mpz_init(d1);
   mpz_init(d2);
   mpz_init(d3);
   mpz_init(d4);
   mpz_init(d5);
   mpz_init(d6);
   mpz_init(d7);

   mpz_set_str(d1,"2", 10);
   mpz_set_str(d2,"521", 10);
   mpz_set_str(d3,"1831", 10);
   mpz_set_str(d4,"3067", 10);
   mpz_set_str(d5,"1294097889777887", 10);
   mpz_set_str(d6,"1838050274902939515372107", 10);
   mpz_set_str(d7,"1384673317831887198890420341", 10);

   gmp_printf("d 1: %Zd\n",d1);
   gmp_printf("d 2: %Zd\n",d2);
   gmp_printf("d 3: %Zd\n",d3);
   gmp_printf("d 4: %Zd\n",d4);
   gmp_printf("d 5: %Zd\n",d5);
   gmp_printf("d 6: %Zd\n",d6);
   gmp_printf("d 7: %Zd\n",d7);

   mpz_t div1, div2, div3, div4, div5, div6, div7;
   mpz_init(div1);
   mpz_init(div2);
   mpz_init(div3);
   mpz_init(div4);
   mpz_init(div5);
   mpz_init(div6);
   mpz_init(div7);


   mpz_t generator, q_1, q;
   mpz_init(generator);
   mpz_set_str(generator, "7977292573950573139348745395838273061335633755132672699089713070964550373066", 10);
   mpz_init(q_1);
   mpz_init(q);
   mpz_set_str(q_1, "19272660807011559256818799230684110654222222307969261359333503297662619279546", 10);
   mpz_set_str(q, "19272660807011559256818799230684110654222222307969261359333503297662619279547", 10);

   gmp_printf("generator: %Zd\n",generator);
   gmp_printf("q: %Zd\n",q);
   gmp_printf("q-1: %Zd\n",q_1);

   mpz_cdiv_q(div1, q_1, d1 );
   mpz_cdiv_q(div2, q_1, d2 );
   mpz_cdiv_q(div3, q_1, d3 );
   mpz_cdiv_q(div4, q_1, d4 );
   mpz_cdiv_q(div5, q_1, d5 );
   mpz_cdiv_q(div6, q_1, d6 );
   mpz_cdiv_q(div7, q_1, d7 );

   gmp_printf("exp 1: %Zd\n",div1);
   gmp_printf("exp 2: %Zd\n",div2);
   gmp_printf("exp 3: %Zd\n",div3);
   gmp_printf("exp 4: %Zd\n",div4);
   gmp_printf("exp 5: %Zd\n",div5);
   gmp_printf("exp 6: %Zd\n",div6);
   gmp_printf("exp 7: %Zd\n",div7);

   mpz_powm(d1, generator, div1, q);
   mpz_powm(d2, generator, div2, q);
   mpz_powm(d3, generator, div3, q);
   mpz_powm(d4, generator, div4, q);
   mpz_powm(d5, generator, div5, q);
   mpz_powm(d6, generator, div6, q);
   mpz_powm(d7, generator, div7, q);

   element_printf("result 1: %Zd\n",d1);
   element_printf("result 2: %Zd\n",d2);
   element_printf("result 3: %Zd\n",d3);
   element_printf("result 4: %Zd\n",d4);
   element_printf("result 5: %Zd\n",d5);
   element_printf("result 6: %Zd\n",d6);
   element_printf("result 7: %Zd\n",d7);


   element_t gen, out;
   element_init_G1(gen,pairing);
   element_set_str(gen, _DECENT_GENERATOR_, 10);
   element_init_G1(out, pairing);
   mpz_t r;
   mpz_init(r);
   mpz_set_str(r, "1208925819614629174702078", 10);

   mpz_set_str(d1,"2", 10);
   mpz_set_str(d2,"17", 10);
   mpz_set_str(d3,"5927", 10);
   mpz_set_str(d4,"4614553", 10);
   mpz_set_str(d5,"1300038369857", 10);
/*   mpz_cdiv_q(div1, r, d1 );
   mpz_cdiv_q(div2, r, d2 );
   mpz_cdiv_q(div3, r, d3 );
   mpz_cdiv_q(div4, r, d4 );
   mpz_cdiv_q(div5, r, d5 ); */

   element_pow_mpz(out, gen, div1);
   element_printf("final result 1: %B", out);
   element_pow_mpz(out, gen, div2);
   element_printf("final result 2: %B", out);
   element_pow_mpz(out, gen, div3);
   element_printf("final result 3: %B", out);
   element_pow_mpz(out, gen, div4);
   element_printf("final result 4: %B", out);
   element_pow_mpz(out, gen, div5);
   element_printf("final result 5: %B", out);



}

int main(int argc, char**argv)
{
//  decent::crypto::aes_key k;
//   for (int i=0; i<CryptoPP::AES::MAX_KEYLENGTH; i++)
//      k.key_byte[i]=i;
 //  test_aes(k);
   cout<<"AES finished \n";
//   test_key_manipulation();
//   test_el_gamal(k);
//   const CryptoPP::Integer secret("12354678979464");
 //  test_shamir(secret);
   test_move();

 //  test_el_gamal(k);
   const CryptoPP::Integer secret("12354678979464");
 //  test_shamir(secret);
   generate_params();
//   test_generator();
 //  test_custody();

}
