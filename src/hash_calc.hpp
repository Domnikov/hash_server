/**
 * @file hash_calc.h
 * @author Domnikov Ivan
 * @brief Wrapper over openssl EVP hash
 *
 */
#pragma once

#include <memory>

#include <openssl/evp.h>


/**
* @brief hash_t class
* @details Must be created for each connection. When new data had come call calc_hash.
* @details When need to get result call get_hex_str. To continue
*/
class hash_t
{
public:
    /** Length of result hash string*/
    constexpr static size_t HAST_STR_LEN = 33;

    /**
    * @brief hast_t constructor
    * @param[in] Connection file descriptor
    */
    hash_t(int fd):m_fd(fd), m_hash(nullptr, &EVP_MD_CTX_free){}


    /**
    * @brief hast_t destructor
    */
    virtual ~hash_t() = default;


    /**
    * @brief Function to add new data to hash function
    * @details When function call first time or after get_hex_str it will initialise new EVP_MD_CTX object
    * @param[in] buffer with new data
    * @param[in] new data length
    */
    void calc_hash(const char* buf, int len)
    {
        if (!m_hash)
        {
            init();
        }
        EVP_DigestUpdate(m_hash.get(), buf, len);
    }


    /**
    * @brief Function to finalize hash calculation, clean EVP_MD_CTX object and store hash as hex string
    * @param[out] buffer for record hex. Must be at least HAST_STR_LEN length
    * @return false if failed, true if success
    */
    bool get_hex_str(char* dst)
    {
        if (!m_hash)
        {
            perror("Attempt to read empty hash");
            return false;
        }

        unsigned int hash_len;
        char hash_str[EVP_MAX_MD_SIZE];
        EVP_DigestFinal_ex(m_hash.get(), (unsigned char*)hash_str, &hash_len);
        m_hash.reset();

        const char hex[] = {"0123456789ABCDEF"};
        for(unsigned int i = 0; i < hash_len; i++)
        {
            *dst++ = hex[0XF & hash_str[i] >>  4];
            *dst++ = hex[0XF & hash_str[i]      ];
        }
        *dst = '\n';
        return true;
    }


    /**
    * @brief Get connection file descriptor
    * @return Connection file descriptor
    */
    int get_fd(){return m_fd;}

private:
    /**
    * @brief clean old EVP_MD_CTX and initialize new
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


    /** Saved file descriptor. Used for thread pool*/
    int m_fd;

    /** hash object*/
    std::unique_ptr<EVP_MD_CTX, decltype(&EVP_MD_CTX_free)> m_hash;
};
