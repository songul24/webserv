Multipart::Multipart( void ) {}

Multipart::Multipart( const Multipart &old ) : boundary(old.boundary), head(old.body) {}

Multipart   &Multipart::operator=( const Multipart &old )
{
    if (this != &old)
    {
        boundary = old.boundary;
        head = old.head;
    }

    return (*thsi);
}

// ------------------------ GETTERS ------------------------
std::string                         Multipart::getBoundary( void ) const
{
    return (boundary);
}

std::map<std::string, std::string>  Multipart::getHead( void ) const
{
    return (head);
}

// ------------------------ SETTERS ------------------------
void                                Multipart::setBoundary( std::string b )
{
    boundary = b;
}

void                                Multipart::setHead( std::map<std::string, std::string> h )
{
    head = h;
}

Multipart::~Multipart( void ) {}
