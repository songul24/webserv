#pragma once

class   Multipart
{
    private:
        std::string                         boundary;
        std::map<std::string, std::string>  head;

    public:
        Multipart( void );
        Multipart( const Multipart &old );
        Multipart   &operator=( const Multipart &old );

        // ------ GETTERS ------
        std::string                         getBoundary( void ) const;
        std::map<std::string, std::string>  getHead( void ) const;
        
        // ------ SETTERS ------
        void                                setBoundary( std::string b );
        void                                setHead( std::map<std::string, std::string> h );

        ~Multipart( void );
};
