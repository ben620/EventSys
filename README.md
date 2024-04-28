c++20, event system, support all type of functions, listen to specific object, or without any object.
simple and efficient runtime type check

listen to event
```
ESI().Register(EventID::NewJob, &s, &r, &Svr::OnReady);
ESI().Register(EventID::NewJob, &s, &s, &Svr::OnReady);
ESI().Register(EventID::OnVoid, &s, &r, &Svr::OnVoid);
ESI().Register(EventID::OnPointer, &s, &r, &Svr::OnPointer);
ESI().Register(EventID::OnConstMethod, &s, &r, &Svr::OnConstMethod);
ESI().Register(EventID::OnStaticMethod, &s, &Svr::OnStaticMethod);
ESI().Register(EventID::OnNoSender, nullptr, &r, &Svr::OnNoSender);
ESI().Register(EventID::OnLambda, nullptr, [](int b) { std::cout << "OnLambda " << b << std::endl; });
ESI().Register(EventID::OnStdFunction, nullptr, std::function([](int b) { std::cout << "OnStdFunction " << b << std::endl; }));
````
send event
```
ESI().Send(EventID::NewJob, &s, 1, std::string("abc"));
ESI().Send(EventID::NewJob, &r, 1, std::string("abc"));
ESI().Send(EventID::OnVoid, &s);
ESI().Send(EventID::OnPointer, &s, "pointer args");
ESI().Send(EventID::OnConstMethod, &s);
ESI().Send(EventID::OnStaticMethod, &s);
ESI().Send(EventID::OnNoSender, &s);
ESI().SendAll(EventID::OnNoSender);
ESI().SendAll(EventID::OnLambda, 1);
ESI().SendAll(EventID::OnStdFunction, 1);
```
