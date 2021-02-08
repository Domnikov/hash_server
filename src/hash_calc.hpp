/**
 * @file hash_calc.h
 * @author Domnikov Ivan
 * @brief Wrapper over openssl EVP hash to provide C++ style interface
 *
 */
#pragma once

#include <memory>
#include <string_view>

#include <openssl/evp.h>


/**
* @brief hash_t class
* @details Must be created for each connection. When new data had come call calc_hash.
* @details When need to get result call get_hex_str. To continue
*/
class hash_t
{
public:
    hash_t():m_hash(nullptr, &EVP_MD_CTX_free){}
    virtual ~hash_t() = default;


    /**
    * @brief Function to process new data to hash function and return
    * @details Buffer must be valid at least untill the end of this function.
    * @param[in] buffer with new data
    */
    void process(std::string_view buffer)
    {
        if (!buffer.size())
        {
            return;
        }

        if (!m_hash)
        {
            init();
        }

        EVP_DigestUpdate(m_hash.get(), buffer.data(), buffer.size());
    }


    /**
    * @brief Function to get calculated hash
    * @details Function to finalize hash calculation, clean EVP_MD_CTX object and store hash as hex string
    * @details string_view will be invalid after deleting abject or relculated another hash.
    * @return Calculated hash as string_view
    */
    std::string_view get_result()
    {
        unsigned int hash_len;
        char hash_str[EVP_MAX_MD_SIZE];
        EVP_DigestFinal_ex(m_hash.get(), (unsigned char*)hash_str, &hash_len);
        m_hash.reset();

        const char hex[] = {"0123456789ABCDEF"};
        char* dst = out_buf;
        for(unsigned int i = 0; i < hash_len; i++)
        {
            *dst++ = hex[0XF & hash_str[i] >>  4];
            *dst++ = hex[0XF & hash_str[i]      ];
        }
        *dst = '\n';
        return {out_buf, HAST_STR_LEN};
    }

private:
    /**
    * @brief Clean old EVP_MD_CTX and initialize new
    */
    void init()
    {
        m_hash.reset(EVP_MD_CTX_create());

        if(!m_hash)
        {
            perror("Error: hash cannot be initialized");
        }
        else
        {
            EVP_DigestInit_ex(m_hash.get(), EVP_md5(), NULL);
        }
    }


    /** hash object*/
    std::unique_ptr<EVP_MD_CTX, decltype(&EVP_MD_CTX_free)> m_hash;

    /** Length of result hash string*/
    const static size_t HAST_STR_LEN = 33;

    /** Output buffer for storing hash*/
    char out_buf[HAST_STR_LEN];
};
