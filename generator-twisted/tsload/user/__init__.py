from storm.locals import *

class User(object):
    __storm_table__ = "user"
    
    id = Int(primary=True)
    
    name = RawStr()
    gecosName = Unicode()
    
    # Auth service used to authentificate user
    authService = Enum(map = {'local': 0
                              })
    authPassword = RawStr()

class Role(object):
    __storm_table__ = "role"
    
    id = Int(primary=True)
    
    # Reference to user
    user_id = Int()
    user = Reference(user_id, User.id)
    
    role = Enum(map = {'admin': 0,
                       'operator': 1,
                       'user': 2})
    
    # Agent's uuid for operator/user roles
    # For admin role this is optional, because admin user
    # may access any agent
    agentUuid = UUID()                  
    
User.roles = ReferenceSet(User.id, Role.user_id)

class UserAuthError(Exception):
    pass