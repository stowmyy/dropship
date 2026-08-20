use std::{collections::HashSet, io, path::PathBuf, str::FromStr};
use wfp::{
    ActionType, AppIdConditionBuilder, FilterBuilder, FilterEngine, FilterEngineBuilder,
    FilterEnumerator, GUID, IpAddressConditionBuilder, Layer, ProviderBuilder, SubLayerBuilder,
    SubLayerEnumerator, Transaction, delete_filter, delete_provider, delete_sublayer,
};

use crate::api;

const DROPSHIP_V3_WFP_PROVIDER_GUID: GUID = GUID::from_u128(0x9f8b0434_3b49_4cb0_9e1a_d1e7ba2e05d2);
const DROPSHIP_V3_WFP_SUBLAYER_GUID: GUID = GUID::from_u128(0x9d313d65_76a7_4213_8967_bd604d64d47c);

// NOTE i could generate filter guids per server using the bit. << u128
// then they would have constant guids

fn guid_equal(a: wfp::GUID, b: wfp::GUID) -> bool {
    a.data1 == b.data1 && a.data2 == b.data2 && a.data3 == b.data3 && a.data4 == b.data4
}

pub struct WfpConnection {
    pub handle: FilterEngine,
}

impl WfpConnection {
    pub fn new(persistent: bool) -> io::Result<Self> {
        let handle = {
            match persistent {
                false => FilterEngineBuilder::default().dynamic().open(),
                true => FilterEngineBuilder::default().open(),
            }?
        };

        Ok(Self { handle })
    }

    // NOTE currently this creates a single rule app1 or app2. syd2 or gen.
    // i would create separate rules for each application/server but there seems no need too :s
    pub fn wipe_all_and_block_servers_for_applications(
        &mut self,
        blocked_servers: &HashSet<api::KnownServer>,
        paths: &HashSet<PathBuf>,
    ) -> io::Result<()> {
        let transaction = Transaction::new(&mut self.handle)?;

        // wipe dropship configuration
        delete_dropship_wfp(&transaction)?;

        if blocked_servers.is_empty() || paths.is_empty() {
            // return Err(io::Error::new(
            //     io::ErrorKind::Other,
            //     "will not block an empty application or server list",
            // ));

            if blocked_servers.is_empty() {
                log::info!("dropship is disabled because no servers are selected");
            }

            // else {
            //     log::info!("dropship is disabled because no applications are selected");
            // }

            transaction.commit()?;

            return Ok(());
        }

        let provider_name = "dropship v3";
        let provider_description = "dropship blocks game servers";

        let sublayer_name = "dropship blocks";
        let sublayer_description = "contains all blocks for dropship";

        // if we need multiple filters, we can delete this
        let filter_name = "game servers";
        let filter_description =
            "contains all game servers blocked by dropship (for some applications)";

        // write new provider and sublayer
        {
            let provider = ProviderBuilder::default()
                .guid(DROPSHIP_V3_WFP_PROVIDER_GUID)
                .name(provider_name)
                .description(provider_description)
                // .persistent() // REVIEW
                // .service_name(service)
            ;

            let sublayer = SubLayerBuilder::default()
                .guid(DROPSHIP_V3_WFP_SUBLAYER_GUID)
                .provider(DROPSHIP_V3_WFP_PROVIDER_GUID)
                .weight(u16::MAX) // REVIEW
                // .persistent() // REVIEW
                .name(sublayer_name)
                .description(sublayer_description);

            provider.add(&transaction)?;
            sublayer.add(&transaction)?;
        }

        // write a dropship filter
        {
            let mut filter_v4 = FilterBuilder::default()
                // TODO try transport/packet in dynamic mode. system wide but more reliable.
                .layer(Layer::ConnectV4)
                .name(filter_name)
                .description(filter_description)
                .action(ActionType::Block)
                // .condition(PortConditionBuilder::remote().equal(80).build())
                .sublayer(DROPSHIP_V3_WFP_SUBLAYER_GUID)
                .provider(DROPSHIP_V3_WFP_PROVIDER_GUID)
                // .guid(guid)
                // .lifetime(wfp::FilterLifetime::Persistent)
                // .weight(weight)
            ;

            let mut filter_v6 = FilterBuilder::default()
                // TODO try transport/packet in dynamic mode. system wide but more reliable.
                .layer(Layer::ConnectV6)
                .name(filter_name)
                .description(filter_description)
                .action(ActionType::Block)
                .sublayer(DROPSHIP_V3_WFP_SUBLAYER_GUID)
                .provider(DROPSHIP_V3_WFP_PROVIDER_GUID);

            // add paths to condition
            for path in paths {
                filter_v4 =
                    filter_v4.condition(AppIdConditionBuilder::default().equal(path)?.build());
                filter_v6 =
                    filter_v6.condition(AppIdConditionBuilder::default().equal(path)?.build());
            }

            let mut block_some_v4 = false;
            let mut block_some_v6 = false;

            // add ips to condition
            for blocked_server in blocked_servers {
                let networks = blocked_server.block.split(',');

                for network in networks {
                    let network = ipnet::IpNet::from_str(network)
                        .map_err(|e| io::Error::new(io::ErrorKind::InvalidInput, e))?;

                    match network {
                        ipnet::IpNet::V4(net) => {
                            block_some_v4 = true;
                            filter_v4 = filter_v4.condition(
                                IpAddressConditionBuilder::remote()
                                    .subnet_v4(net.addr(), net.prefix_len())
                                    .build(),
                            )
                        }

                        ipnet::IpNet::V6(net) => {
                            block_some_v6 = true;
                            filter_v6 = filter_v6.condition(
                                IpAddressConditionBuilder::remote()
                                    .subnet_v6(net.addr(), net.prefix_len())
                                    .build(),
                            )
                        }
                    };
                }
            }

            if block_some_v4 {
                filter_v4.add(&transaction)?;
            }

            if block_some_v6 {
                filter_v6.add(&transaction)?;
            }
        }
        transaction.commit()?;
        log::info!(
            "dropship is blocking {}",
            format!(
                "{:?}",
                &blocked_servers
                    .iter()
                    .map(|s| &s.token)
                    .collect::<Vec<_>>()
                    .clone()
            )
        );

        Ok(())
    }
}

///
pub fn delete_dropship_wfp(transaction: &Transaction) -> io::Result<()> {
    // delete dropship filters
    {
        let dropship_filters = {
            let mut filter_enum = FilterEnumerator::new(&transaction)?;

            let mut ids = vec![];

            while let Some(filter) = filter_enum.next() {
                let filter = filter?;
                let id = filter.id();

                if filter
                    .provider()
                    .is_some_and(|p| guid_equal(p, DROPSHIP_V3_WFP_PROVIDER_GUID))
                {
                    ids.push(id);
                }
            }

            ids
        };

        for id in dropship_filters {
            if let Err(e) = delete_filter(&transaction, id) {
                log::error!("{}", e);
            };
        }
    }

    // delete dropship sublayers
    {
        let dropship_sublayers = {
            let mut sublayer_enum = SubLayerEnumerator::new(&transaction)?;

            let mut ids = vec![];

            while let Some(sublayer) = sublayer_enum.next() {
                let sublayer = sublayer?;
                let id = sublayer.guid();

                if sublayer
                    .provider()
                    .is_some_and(|p| guid_equal(p, DROPSHIP_V3_WFP_PROVIDER_GUID))
                {
                    ids.push(id);
                }
            }

            ids
        };

        for guid in dropship_sublayers {
            if let Err(e) = delete_sublayer(&transaction, &guid) {
                log::error!("{}", e);
            };
        }
    }

    let _ = delete_provider(&transaction, &DROPSHIP_V3_WFP_PROVIDER_GUID);

    Ok(())
}
