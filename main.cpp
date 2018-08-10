//
// Created by qiyu on 4/24/18.
//
#include <iostream>
#include "feather.h"
#include "feather_cfg.hpp"
#include "entity.h"
#include "dao.hpp"
#include "user_controller.hpp"
#include "article_controller.hpp"
#include "upload_controller.hpp"
#include "static_res_controller.hpp"

using namespace feather;
using namespace ormpp;
using namespace cinatra;

void init_db(bool clear_all){
    dao_t<dbng<mysql>> dao;
    bool r = true;
    if(clear_all){
        bool r1 = dao.drop_table<user>();
        bool r2 = dao.drop_table<article>();
        bool r3 = dao.drop_table<article_detail>();
        assert(r1&&r2&&r3);
    }

    {
        bool r1 = dao.create_table<user>({ SID(user::id) });
        bool r2 = dao.create_table<article>({ SID(article::id) });
        bool r3 = dao.create_table<article_detail>({ SID(article_detail::id) });
        assert(r1&&r2&&r3);
    }

//    {
//        //add some test data to tables
//        user u1{0, "tom"};
//        user u2{0, "mike"};
//        std::vector<user> v{u1, u2};
//        int n = dao.add_objects(v);
//        assert(n==2);
//        bool r1 = dao.add_object(u1);
//        bool r2 = dao.add_object(u2);
//    }
//
//    {
//        article ar1{0, "title1", "test", 1, 1, get_cur_time_str()};
//        article ar2{0, "title2", "hello", 2, 1, get_cur_time_str()};
//        bool r1 = dao.add_object(ar1);
//        bool r2 = dao.add_object(ar2);
//        assert(r1&&r2);
//
//        article_detail detail1{0, ar1.id, "title1", "it is a test", get_cur_time_str()};
//        article_detail detail2{0, ar2.id, "title2", "hello world", get_cur_time_str()};
//        r1 = dao.add_object(detail1);
//        r2 = dao.add_object(detail2);
//        assert(r1&&r2);
//    }
//
//    {
//        std::vector<user> v;
//        bool r = dao.get_object(v, "id<3");
//        assert(r);
//
//        std::vector<article> v1;
//        bool r1 = dao.get_object(v1);
//        assert(r1);
//    }
}

void init(const feather_cfg& cfg){

    nanolog::initialize(nanolog::GuaranteedLogger(), cfg.log_path, cfg.log_name, cfg.roll_file_size);

    dao_t<dbng<mysql>>::init(cfg.db_conn_num, cfg.db_ip.data(), cfg.user_name.data(), cfg.pwd.data(),
                             cfg.db_name.data(), cfg.timeout);
    init_db(cfg.drop_all_table);
}

int main(){
    feather_cfg cfg{};
    bool ret = config_manager::from_file(cfg, "./cfg/feather.cfg");
    if (!ret) {
        return -1;
    }

    init(cfg);

    cinatra::http_server server(cfg.thread_num);
    server.set_static_dir("/static/");
	server.enable_http_cache(true);//set global cache
    bool r = server.listen("0.0.0.0", cfg.port);
    if (!r) {
        LOG_INFO << "listen failed";
        return -1;
	}

	server.set_http_handler<GET>("/home", [](request& req, response& res) {
		/*{
			dao_t<dbng<mysql>> dao;
			std::vector<pp_post_views> v;
			dao.get_object(v, "limit 4");
		}
		{
			dao_t<dbng<mysql>> dao;
			std::vector<pp_comment> v;
			dao.get_object(v, "limit 4");
		}
		{
			dao_t<dbng<mysql>> dao;
			std::vector<pp_user> v;
			dao.get_object(v, "limit 4");
		}*/
		dao_t<dbng<mysql>> dao;
		std::vector<pp_posts> v;
		dao.get_object(v, "post_status='publish'", "order by post_date desc", "limit 4");

		nlohmann::json article_list;
		for (auto& post : v) {
			nlohmann::json item;
			item["post_author"] = post.post_author;
			item["content_abstract"] = post.content_abstract;
			item["post_date"] = post.post_date;
			item["post_title"] = post.post_title;
			article_list.push_back(item);
		}
		
		nlohmann::json result;
		result["article_list"] = article_list;
		res.add_header("Content-Type", "text/html; charset=utf-8");
		res.set_status_and_content(status_type::ok, render::render_file("./purecpp/html/home.html", result));
	}, enable_cache{ false });

    user_controller user_ctl;
    server.set_http_handler<POST>("/add_user", &user_controller::add_user, &user_ctl);

    article_controller article_ctl;
    server.set_http_handler<GET, POST>("/", &article_controller::index, &article_ctl);
    server.set_http_handler<POST>("/add_article", &article_controller::add_article, &article_ctl);
    server.set_http_handler<GET, POST>("/get_article_list", &article_controller::get_article_list, &article_ctl);
    server.set_http_handler<GET, POST>("/get_article_detail", &article_controller::get_article_detail, &article_ctl);
    server.set_http_handler<GET, POST>("/remove_article", &article_controller::remove_article, &article_ctl);
    server.set_http_handler<POST>("/update_article", &article_controller::update_article, &article_ctl);

    upload_controller up;
    server.set_http_handler<POST>("/upload", &upload_controller::upload, &up);

    server.run();

    return 0;
}
